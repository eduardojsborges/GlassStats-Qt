#pragma once
#include <QColor>
#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QRectF>
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <optional>

namespace gs {
enum class Availability { Available, WarmingUp, Unavailable, Stale };
struct Reading {
    std::optional<double> value;
    QString unit, detail;
    Availability status = Availability::Unavailable;
    static Reading of(double value, QString unit, QString detail = {});
    static Reading missing(QString unit, QString detail = {}, Availability state = Availability::Unavailable);
    QVariantMap variant() const;
};
struct Adapter { QString id, name, persistentId; quint64 dedicatedBytes = 0; bool discrete = false; };
struct Sensor { QString id, device, name, kind; std::optional<double> celsius; };
struct Sample {
    QDateTime at;
    QString cpuName, gpuId, gpuName, memoryModel = "dedicated";
    Reading cpu, gpu, ramUsed, ramTotal, vramUsed, vramTotal, cpuTemperature, gpuTemperature;
    QVariantMap variant(const QString& temperatureUnit) const;
};
struct Capability { QString state, reason; QVariantMap variant() const { return {{"state",state},{"reason",reason}}; } };
using Capabilities = QMap<QString,Capability>;
struct Palette { QColor surface, text, secondary; double opacity; };
double luminance(QColor color);
double contrast(QColor a, QColor b);
QColor blend(QColor a, QColor b, double fraction);
double worstContrast(QColor text, QColor surface, double opacity);
Palette palette(const QVariantMap& settings, bool systemDark);
struct ContrastResult { QColor color; bool halo; };
class AdaptiveContrast {
    std::optional<QColor> current_, pending_; int confirmations_ = 0;
public:
    ContrastResult select(QColor preferred, const QVector<QColor>& surfaces);
};
struct Monitor { QRect bounds, work; double dpi = 96; };
struct Placement { QRect rect; double radius = 0, pixels = 1; };
Monitor selectMonitor(const QVector<Monitor>& monitors, QPoint anchor);
Placement fit(const QVector<Monitor>& monitors, std::optional<QPoint> anchor, QSizeF logical, double scale, double radius, QPoint cursor);
Placement fitHost(QRect host, QPoint anchor, QSizeF logical, double pixels, double radius);
QPointF relativeAnchor(QRect host, QPoint anchor);
QPoint absoluteAnchor(QRect host, QPointF relative);
bool fullyCovered(QRect card, const QVector<QRect>& covers);
class History {
public:
    struct Point { QDateTime at; std::optional<double> value; };
    QVector<Point> points;
    void add(QDateTime at, std::optional<double> value);
    QVariantList variant(QDateTime now) const;
};
std::optional<double> memoryRatio(const Reading& used, const Reading& total);
std::optional<QPair<QString,QString>> parseGpuCounter(QString name);
std::optional<double> busiestEngine(const QMap<QString,double>& counters, const QString& adapter);
struct Shortcut { quint32 modifiers, key; };
std::optional<Shortcut> parseShortcut(QString text);
struct Version {
    int major=0,minor=0,patch=0; QString pre;
    static std::optional<Version> parse(QString text);
    int compare(const Version& other) const;
};
QVariantMap defaults();
QVariantMap normalize(QVariantMap settings);
QVariantMap migrateLegacy(QJsonObject object);
QVariantMap loadSettings(const QString& path, QString* error = nullptr);
bool saveSettings(const QString& path, const QVariantMap& settings, QString* error = nullptr);
Reading temperature(const QVector<Sensor>& sensors, QDateTime at, QString kind, QString device, QString id, QDateTime now);
QString platformKey();
QString assetName(const QString& tag, const QString& platform = platformKey());
struct Release { QString tag, notes, page, assetUrl, assetName, digest, checksumUrl; qint64 size = 0; };
std::optional<Release> selectRelease(const QJsonArray& releases, QString installed, bool beta, QString platform);
}
