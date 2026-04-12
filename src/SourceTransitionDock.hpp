#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-scene.h>

class SourceTransitionDock : public QWidget {
    Q_OBJECT

public:
    explicit SourceTransitionDock(QWidget *parent = nullptr);
    ~SourceTransitionDock();

private slots:
    void refreshSelectedSources();
    void onShowTransitionChanged();
    void onHideTransitionChanged();
    void onApplyToAll();

private:
    void setupUI();
    void loadTransitionsForSource(obs_source_t *source);
    void applyTransitionToSource(obs_source_t *source,
                                  const QString &showId, int showDuration,
                                  const QString &hideId, int hideDuration);

    static void frontendEventCallback(obs_frontend_event event, void *data);

    QVBoxLayout  *mainLayout      = nullptr;
    QLabel       *placeholderLabel = nullptr;
    QWidget      *controlsWidget  = nullptr;
    QVBoxLayout  *controlsLayout  = nullptr;
    QComboBox    *showTransition  = nullptr;
    QSpinBox     *showDuration    = nullptr;
    QComboBox    *hideTransition  = nullptr;
    QSpinBox     *hideDuration    = nullptr;
    QPushButton  *applyAllButton  = nullptr;

    QList<obs_source_t *> selectedSources;
};
