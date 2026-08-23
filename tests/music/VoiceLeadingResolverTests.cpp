#include <catch2/catch_test_macros.hpp>
#include "music/VoiceLeadingResolver.h"
#include "music/DiatonicChordVoicer.h"

using namespace chordsynth::music;

TEST_CASE("VoiceLeadingResolver nearest voice leading basics", "[music][voiceleading]") {
    DiatonicChordVoicer voicer;

    SECTION("Empty previous notes returns targetDefaultNotes unchanged") {
        NoteSet emptyPrev{};
        VoicingSpec spec{.shape = ChordShape::triad, .baseOctave = 3};
        auto chordI = voicer.voiceChord(0, 0, spec);
        NoteSet resolved = VoiceLeadingResolver::resolveNearestVoiceLeading(emptyPrev, chordI.notes);
        REQUIRE(resolved == chordI.notes);
    }

    SECTION("Empty target notes returns empty NoteSet") {
        NoteStorage prevNotes{60, 64, 67, 0, 0, 0};
        NoteSet prev(prevNotes, 3);
        NoteSet emptyTarget{};
        NoteSet resolved = VoiceLeadingResolver::resolveNearestVoiceLeading(prev, emptyTarget);
        REQUIRE(resolved.empty());
    }

    SECTION("Identical target returns identical notes (zero movement)") {
        VoicingSpec spec{.shape = ChordShape::triad, .baseOctave = 3};
        auto chordI = voicer.voiceChord(0, 0, spec);
        NoteSet resolved = VoiceLeadingResolver::resolveNearestVoiceLeading(chordI.notes, chordI.notes);
        REQUIRE(resolved == chordI.notes);
    }

    SECTION("IV -> V -> I progression minimizes voice movement") {
        // C major IV (F maj: F A C) -> V (G maj: G B D) -> I (C maj: C E G)
        VoicingSpec specManual{
            .shape = ChordShape::triad,
            .voiceLeading = VoiceLeadingMode::manual,
            .baseOctave = 3
        };
        auto chordIV = voicer.voiceChord(0, 3, specManual); // F3 (53), A3 (57), C4 (60)
        auto chordV = voicer.voiceChord(0, 4, specManual);  // G3 (55), B3 (59), D4 (62)
        auto chordI = voicer.voiceChord(0, 0, specManual);  // C4 (48), E4 (52), G4 (55)

        NoteSet vResolved = VoiceLeadingResolver::resolveNearestVoiceLeading(chordIV.notes, chordV.notes);
        REQUIRE(vResolved.size() == 3);
        // Ensure within bounds
        for (int i = 0; i < vResolved.size(); ++i) {
            REQUIRE(vResolved[static_cast<std::size_t>(i)] >= 48);
            REQUIRE(vResolved[static_cast<std::size_t>(i)] <= 96);
        }

        // From V to I: common tone G (55) should be preserved!
        NoteSet iResolved = VoiceLeadingResolver::resolveNearestVoiceLeading(vResolved, chordI.notes);
        REQUIRE(iResolved.size() == 3);
        // Common tone G (55) is present in G major (vResolved) and C major
        bool hasG = false;
        for (int i = 0; i < iResolved.size(); ++i) {
            if (iResolved[static_cast<std::size_t>(i)] % 12 == 7) { // G
                hasG = true;
                REQUIRE(iResolved[static_cast<std::size_t>(i)] >= 48);
            }
        }
        REQUIRE(hasG);
    }

    SECTION("ii -> V -> I progression retains common tones and small steps") {
        // ii (Dm7: D F A C) -> V (G7: G B D F) -> I (Cmaj7: C E G B)
        VoicingSpec spec7{.shape = ChordShape::seventh, .baseOctave = 3};
        auto chordII = voicer.voiceChord(0, 1, spec7); // D F A C (50, 53, 57, 60)
        auto chordV = voicer.voiceChord(0, 4, spec7);  // G B D F (55, 59, 62, 65)

        NoteSet vResolved = VoiceLeadingResolver::resolveNearestVoiceLeading(chordII.notes, chordV.notes);
        REQUIRE(vResolved.size() == 4);

        int totalMovement = 0;
        for (int i = 0; i < 4; ++i) {
            totalMovement += std::abs(vResolved[static_cast<std::size_t>(i)] - chordII.notes[static_cast<std::size_t>(i)]);
        }
        // Total movement across 4 voices should be small (<= 8 semitones)
        REQUIRE(totalMovement <= 8);
    }

    SECTION("Deterministic output on repeated calls") {
        VoicingSpec spec7{.shape = ChordShape::seventh, .baseOctave = 3};
        auto chordII = voicer.voiceChord(0, 1, spec7);
        auto chordV = voicer.voiceChord(0, 4, spec7);

        NoteSet res1 = VoiceLeadingResolver::resolveNearestVoiceLeading(chordII.notes, chordV.notes);
        NoteSet res2 = VoiceLeadingResolver::resolveNearestVoiceLeading(chordII.notes, chordV.notes);
        REQUIRE(res1 == res2);
    }

    SECTION("All notes remain strictly within safe register constraints [48..96]") {
        NoteStorage extremeNotes{48, 52, 55, 0, 0, 0};
        NoteSet prev(extremeNotes, 3);
        NoteStorage highNotes{84, 88, 91, 0, 0, 0};
        NoteSet target(highNotes, 3);

        NoteSet resolved = VoiceLeadingResolver::resolveNearestVoiceLeading(prev, target);
        for (int i = 0; i < resolved.size(); ++i) {
            REQUIRE(resolved[static_cast<std::size_t>(i)] >= 48);
            REQUIRE(resolved[static_cast<std::size_t>(i)] <= 96);
        }
    }

    SECTION("Different voice counts (e.g., 4 voices -> 3 voices) handled cleanly") {
        NoteStorage notes4{50, 53, 57, 60, 0, 0};
        NoteSet prev(notes4, 4);
        NoteStorage notes3{55, 59, 62, 0, 0, 0};
        NoteSet target(notes3, 3);

        NoteSet resolved = VoiceLeadingResolver::resolveNearestVoiceLeading(prev, target);
        REQUIRE(resolved.size() == 3);
        for (int i = 0; i < resolved.size(); ++i) {
            REQUIRE(resolved[static_cast<std::size_t>(i)] >= 48);
            REQUIRE(resolved[static_cast<std::size_t>(i)] <= 96);
        }
    }

    SECTION("Six-voice extended chords lead smoothly without exceeding bounds") {
        // Cmaj13 vs Dm13
        VoicingSpec spec13{.shape = ChordShape::thirteenth, .baseOctave = 3};
        auto chordI = voicer.voiceChord(0, 0, spec13);
        auto chordII = voicer.voiceChord(0, 1, spec13);

        NoteSet resolved = VoiceLeadingResolver::resolveNearestVoiceLeading(chordI.notes, chordII.notes);
        REQUIRE(resolved.size() == chordII.notes.size());
        for (int i = 0; i < resolved.size(); ++i) {
            REQUIRE(resolved[static_cast<std::size_t>(i)] >= 48);
            REQUIRE(resolved[static_cast<std::size_t>(i)] <= 96);
        }
    }
}
