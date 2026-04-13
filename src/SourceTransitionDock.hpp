#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>

struct TransitionClipboard {
    QString typeId;
    int     duration = 300;
    bool    hasData  = false;
};

class SourceTransitionDock : public QWidget {
    Q_OBJECT

public:
    explicit SourceTransitionDock(QWidget *parent = nullptr);
    ~SourceTransitionDock();

    void injectIntoSourcesDock();

private slots:
    void onSceneChanged();
    void cleanup();
    void refreshSelectedSources();
    void onShowTransitionChanged();
    void onHideTransitionChanged();
    void onShowDurationChanged();
    void onHideDurationChanged();
    void onShowProperties();
    void onHideProperties();
    void onCopyShow();
    void onCopyHide();
    void onPasteShow();
    void onPasteHide();
    void onApplyToScene();

private:
    void setupUI();
    void connectSceneSignals(obs_source_t *scene_source);
    void disconnectSceneSignals();
    void loadTransitionsForItem(obs_sceneitem_t *item);
    void applyTransitionToItem(obs_sceneitem_t *item,
                               const QString &showId, int showDur,
                               const QString &hideId, int hideDur);
    void openProperties(bool show);
    QPushButton *makeIconButton(QStyle::StandardPixmap icon, const QString &tooltip);

    static void frontendEventCallback(obs_frontend_event event, void *data);

    // UI
    QVBoxLayout  *mainLayout        = nullptr;
    QLabel       *placeholderLabel  = nullptr;
    QWidget      *controlsWidget    = nullptr;
    QVBoxLayout  *controlsLayout    = nullptr;
    QComboBox    *showTransition    = nullptr;
    QSpinBox     *showDuration      = nullptr;
    QComboBox    *hideTransition    = nullptr;
    QSpinBox     *hideDuration      = nullptr;
    QPushButton  *applySceneButton  = nullptr;

    // State
    obs_source_t             *currentScene = nullptr;
    QList<obs_sceneitem_t *>  selectedItems;
    TransitionClipboard       showClipboard;
    TransitionClipboard       hideClipboard;
};
