#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace juce
{

class ChordSynthStandaloneApp final : public JUCEApplication
{
public:
    ChordSynthStandaloneApp()
    {
        PropertiesFile::Options options;
        options.applicationName = CharPointer_UTF8 (JucePlugin_Name);
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        options.folderName = "~/.config";
       #endif
        appProperties.setStorageParameters (options);
    }

    const String getApplicationName() override { return CharPointer_UTF8 (JucePlugin_Name); }
    const String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted (const String&) override {}

    void initialise (const String&) override
    {
        auto* settings = appProperties.getUserSettings();
        if (settings->getBoolValue ("startupInProgress", false))
        {
            const auto previousSettings = settings->getFile();
            if (previousSettings.existsAsFile())
                previousSettings.copyFileTo(previousSettings.getSiblingFile(
                    previousSettings.getFileNameWithoutExtension() + ".recovery-backup.settings"));

            settings->clear();
            settings->saveIfNeeded();
        }

        settings->setValue ("startupInProgress", true);
        settings->saveIfNeeded();

        mainWindow = rawToUniquePtr (new StandaloneFilterWindow (
            getApplicationName(),
            LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
            createPluginHolder()));
        mainWindow->setVisible (true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        pluginHolder = nullptr;

        if (auto* settings = appProperties.getUserSettings())
            settings->setValue ("startupInProgress", false);
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
            mainWindow->pluginHolder->savePluginState();
        else if (pluginHolder != nullptr)
            pluginHolder->savePluginState();
        quit();
    }

private:
    std::unique_ptr<StandalonePluginHolder> createPluginHolder()
    {
        return std::make_unique<StandalonePluginHolder> (appProperties.getUserSettings(), false);
    }

    ApplicationProperties appProperties;
    std::unique_ptr<StandaloneFilterWindow> mainWindow;
    std::unique_ptr<StandalonePluginHolder> pluginHolder;
};

} // namespace juce

JUCE_CREATE_APPLICATION_DEFINE (juce::ChordSynthStandaloneApp)
