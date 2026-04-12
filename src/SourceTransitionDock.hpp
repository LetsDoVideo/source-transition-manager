#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QScrollArea>
#include <obs-frontend-api.h>
#include <obs.h>

class SourceTransitionDock : public QWidget {
    Q_OBJECT

public:
    explicit SourceTransitionDock(QWidget *parent = nullptr);
    ~SourceTransitionDock();

private slots:
    void onSceneItemSelectionChanged();
    void onShowTransitionChanged();
    void onHideTransitionChanged();
    void onApplyToAll();

private:
    void setupUI();
    void refreshSelectedSources();
    void loadTransitionsForSource(obs_source_t *source);
    void applyTransitionToSource(obs_source_t *source,
                                  const QString &showId, int showDuration,
                                  const QString &hideId, int hideDuration);

    static void frontendEventCallback(obs_frontend_event event, void *data);

    // UI elements
    QVBoxLayout    *mainLayout      = nullptr;
    QLabel         *placeholderLabel = nullptr;
    QScrollArea    *scrollArea      = nullptr;
    QWidget        *scrollContents  = nullptr;
    QVBoxLayout    *scrollLayout    = nullptr;

    // Per-source controls (shown when sources are selected)
    QWidget        *controlsWidget  = nullptr;
    QVBoxLayout    *controlsLayout  = nullptr;
    QComboBox      *showTransition  = nullptr;
    QSpinBox       *showDuration    = nullptr;
    QComboBox      *hideTransition  = nullptr;
    QSpinBox       *hideDuration    = nullptr;
    QPushButton    *applyAllButton  = nullptr;

    // Currently selected sources
    QList<obs_source_t *> selectedSources;
};
