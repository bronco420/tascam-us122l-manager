#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QDir>
#include <QSettings>
#include <QFile>

class Config {
public:
    explicit Config();

    // Singleton accessor (used by the GUI)
    static Config& instance();

    // Paths
    QString getAppDir() const;
    QString getSettingsDir() const;
    QString getAsoundrcPath() const;
    QString getLauncherPath() const;

    // Settings
    QString getSampleRate() const;
    void setSampleRate(const QString& sr);

    QString getBufferSize() const;
    void setBufferSize(const QString& buf);

    QString getPeriods() const;
    void setPeriods(const QString& per);

    bool asoundrcExists() const;
    bool autostartEnabled() const;
    void setAutostart(bool enabled);

    int getSourceVolumeDefault() const;
    void setSourceVolumeDefault(int percent);

    // Auto-start
    QString getAutostartUnit() const;
    QString getAutostartScript() const;

    // Assets
    QString getIconsDir() const;
    QString getLogoPath() const;
    QString getProductDir() const;
    QString getTascamLogoPath() const;

    // State
    QString getStateFilePath() const;
    QString getLogFilePath() const;
    QString getLockFilePath() const;
    QString getActionLockFilePath() const;

private:
    QString m_appDir;
    QString m_settingsDir;
    QString m_iconsDir;
    QString m_productDir;
    QString m_launcherPath;

    QString findLauncherPath() const;

    void initPaths();
    void initSettings();
};

#endif // CONFIG_H