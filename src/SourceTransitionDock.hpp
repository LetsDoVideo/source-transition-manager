#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <util/platform.h>

class SourceTransitionDock : public QWidget {
    Q_OBJECT

public:
    explicit SourceTransitionDock(QWidget *parent = nullptr);
    ~SourceTransitionDock();

private slots:
    void onSceneChanged();
    void refreshSelectedSources();
    void onShowTransitionChanged();
    void onHideTransitionChanged();
    void onApplyToAll();

private:
    void setupUI();
    void connectSceneSignals(obs_source_t *scene_source);
    void disconnectSceneSignals();
    void loadTransitionsForItem(obs_sceneitem_t *item);
    void applyTransitionToItem(obs_sceneitem_t *item,
                               const QString &showId, int showDuration,
                               const QString &hideId, int hideDuration);

    static void frontendEventCallback(obs_frontend_event event, void *data);

    QVBoxLayout  *mainLayout       = nullptr;
    QLabel       *placeholderLabel = nullptr;
    QWidget      *controlsWidget   = nullptr;
    QVBoxLayout  *controlsLayout   = nullptr;
    QComboBox    *showTransition   = nullptr;
    QSpinBox     *showDuration     = nullptr;
    QComboBox    *hideTransition   = nullptr;
    QSpinBox     *hideDuration     = nullptr;
    QPushButton  *applyAllButton   = nullptr;

    obs_source_t            *currentScene = nullptr;
    QList<obs_sceneitem_t *> selectedItems;
};
