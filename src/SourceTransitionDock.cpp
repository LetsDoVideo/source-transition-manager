#include "SourceTransitionDock.hpp"
#include <obs-module.h>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGroupBox>

// Available transition types in OBS
static const QList<QPair<QString, QString>> TRANSITION_TYPES = {
    {"cut_transition",   "Cut"},
    {"fade_transition",  "Fade"},
    {"swipe_transition", "Swipe"},
    {"slide_transition", "Slide"},
    {"obs_stinger_transition", "Stinger"},
    {"fade_to_color_transition", "Fade to Color"},
    {"wipe_transition",  "Luma Wipe"},
};

SourceTransitionDock::SourceTransitionDock(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    obs_frontend_add_event_callback(frontendEventCallback, this);
}

SourceTransitionDock::~SourceTransitionDock()
{
    obs_frontend_remove_event_callback(frontendEventCallback, this);
}

void SourceTransitionDock::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Placeholder shown when nothing is selected
    placeholderLabel = new QLabel("Select a source to see transitions", this);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setWordWrap(true);
    placeholderLabel->setStyleSheet("color: gray; padding: 12px;");
    mainLayout->addWidget(placeholderLabel);

    // Controls widget shown when sources are selected
    controlsWidget = new QWidget(this);
    controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(6);

    // Show transition group
    QGroupBox *showGroup = new QGroupBox("Show Transition", controlsWidget);
    QVBoxLayout *showLayout = new QVBoxLayout(showGroup);

    showTransition = new QComboBox(showGroup);
    for (auto &t : TRANSITION_TYPES)
        showTransition->addItem(t.second, t.first);

    showDuration = new QSpinBox(showGroup);
    showDuration->setRange(0, 10000);
    showDuration->setSingleStep(100);
    showDuration->setSuffix(" ms");
    showDuration->setValue(300);

    QHBoxLayout *showDurLayout = new QHBoxLayout();
    showDurLayout->addWidget(new QLabel("Type:", showGroup));
    showDurLayout->addWidget(showTransition, 1);
    showLayout->addLayout(showDurLayout);

    QHBoxLayout *showDurLayout2 = new QHBoxLayout();
    showDurLayout2->addWidget(new QLabel("Duration:", showGroup));
    showDurLayout2->addWidget(showDuration, 1);
    showLayout->addLayout(showDurLayout2);

    controlsLayout->addWidget(showGroup);

    // Hide transition group
    QGroupBox *hideGroup = new QGroupBox("Hide Transition", controlsWidget);
    QVBoxLayout *hideLayout = new QVBoxLayout(hideGroup);

    hideTransition = new QComboBox(hideGroup);
    for (auto &t : TRANSITION_TYPES)
        hideTransition->addItem(t.second, t.first);

    hideDuration = new QSpinBox(hideGroup);
    hideDuration->setRange(0, 10000);
    hideDuration->setSingleStep(100);
    hideDuration->setSuffix(" ms");
    hideDuration->setValue(300);

    QHBoxLayout *hideDurLayout = new QHBoxLayout();
    hideDurLayout->addWidget(new QLabel("Type:", hideGroup));
    hideDurLayout->addWidget(hideTransition, 1);
    hideLayout->addLayout(hideDurLayout);

    QHBoxLayout *hideDurLayout2 = new QHBoxLayout();
    hideDurLayout2->addWidget(new QLabel("Duration:", hideGroup));
    hideDurLayout2->addWidget(hideDuration, 1);
    hideLayout->addLayout(hideDurLayout2);

    controlsLayout->addWidget(hideGroup);

    // Apply to all button
    applyAllButton = new QPushButton("Apply to All Selected Sources", controlsWidget);
    controlsLayout->addWidget(applyAllButton);

    controlsLayout->addStretch();
    mainLayout->addWidget(controlsWidget);

    // Start with controls hidden
    controlsWidget->hide();

    // Connect signals
    connect(showTransition, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SourceTransitionDock::onShowTransitionChanged);
    connect(hideTransition, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SourceTransitionDock::onHideTransitionChanged);
    connect(applyAllButton, &QPushButton::clicked,
            this, &SourceTransitionDock::onApplyToAll);
}

void SourceTransitionDock::frontendEventCallback(obs_frontend_event event, void *data)
{
    auto *dock = static_cast<SourceTransitionDock *>(data);

    switch (event) {
    case OBS_FRONTEND_EVENT_SCENE_ITEM_SELECTED:
    case OBS_FRONTEND_EVENT_SCENE_CHANGED:
        QMetaObject::invokeMethod(dock, "refreshSelectedSources",
                                  Qt::QueuedConnection);
        break;
    default:
        break;
    }
}

void SourceTransitionDock::refreshSelectedSources()
{
    // Release previously held sources
    for (auto *src : selectedSources)
        obs_source_release(src);
    selectedSources.clear();

    obs_source_t *scene_source = obs_frontend_get_current_scene();
    if (!scene_source) {
        placeholderLabel->show();
        controlsWidget->hide();
        return;
    }

    obs_scene_t *scene = obs_scene_from_source(scene_source);
    obs_source_release(scene_source);

    // Collect selected scene items
    obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *data) -> bool {
        if (obs_sceneitem_selected(item)) {
            auto *dock = static_cast<SourceTransitionDock *>(data);
            obs_source_t *src = obs_sceneitem_get_source(item);
            obs_source_addref(src);
            dock->selectedSources.append(src);
        }
        return true;
    }, this);

    if (selectedSources.isEmpty()) {
        placeholderLabel->show();
        controlsWidget->hide();
        return;
    }

    placeholderLabel->hide();
    controlsWidget->show();

    // Load transitions from the first selected source
    loadTransitionsForSource(selectedSources.first());
}

void SourceTransitionDock::loadTransitionsForSource(obs_source_t *source)
{
    obs_source_t *showTr = obs_source_get_show_transition(source);
    obs_source_t *hideTr = obs_source_get_hide_transition(source);

    // Block signals while we update UI
    showTransition->blockSignals(true);
    hideTransition->blockSignals(true);
    showDuration->blockSignals(true);
    hideDuration->blockSignals(true);

    if (showTr) {
        const char *id = obs_source_get_id(showTr);
        int idx = showTransition->findData(QString(id));
        if (idx >= 0) showTransition->setCurrentIndex(idx);
        showDuration->setValue(obs_source_get_show_transition_duration(source));
        obs_source_release(showTr);
    } else {
        showTransition->setCurrentIndex(0); // Cut
        showDuration->setValue(300);
    }

    if (hideTr) {
        const char *id = obs_source_get_id(hideTr);
        int idx = hideTransition->findData(QString(id));
        if (idx >= 0) hideTransition->setCurrentIndex(idx);
        hideDuration->setValue(obs_source_get_hide_transition_duration(source));
        obs_source_release(hideTr);
    } else {
        hideTransition->setCurrentIndex(0); // Cut
        hideDuration->setValue(300);
    }

    showTransition->blockSignals(false);
    hideTransition->blockSignals(false);
    showDuration->blockSignals(false);
    hideDuration->blockSignals(false);
}

void SourceTransitionDock::applyTransitionToSource(obs_source_t *source,
    const QString &showId, int showDur,
    const QString &hideId, int hideDur)
{
    // Show transition
    obs_source_t *showTr = obs_source_create_private(
        showId.toUtf8().constData(), nullptr, nullptr);
    if (showTr) {
        obs_source_set_show_transition(source, showTr);
        obs_source_set_show_transition_duration(source, showDur);
        obs_source_release(showTr);
    }

    // Hide transition
    obs_source_t *hideTr = obs_source_create_private(
        hideId.toUtf8().constData(), nullptr, nullptr);
    if (hideTr) {
        obs_source_set_hide_transition(source, hideTr);
        obs_source_set_hide_transition_duration(source, hideDur);
        obs_source_release(hideTr);
    }
}

void SourceTransitionDock::onShowTransitionChanged()
{
    if (selectedSources.isEmpty()) return;

    QString id = showTransition->currentData().toString();
    int dur = showDuration->value();

    for (auto *src : selectedSources) {
        obs_source_t *tr = obs_source_create_private(
            id.toUtf8().constData(), nullptr, nullptr);
        if (tr) {
            obs_source_set_show_transition(src, tr);
            obs_source_set_show_transition_duration(src, dur);
            obs_source_release(tr);
        }
    }
}

void SourceTransitionDock::onHideTransitionChanged()
{
    if (selectedSources.isEmpty()) return;

    QString id = hideTransition->currentData().toString();
    int dur = hideDuration->value();

    for (auto *src : selectedSources) {
        obs_source_t *tr = obs_source_create_private(
            id.toUtf8().constData(), nullptr, nullptr);
        if (tr) {
            obs_source_set_hide_transition(src, tr);
            obs_source_set_hide_transition_duration(src, dur);
            obs_source_release(tr);
        }
    }
}

void SourceTransitionDock::onApplyToAll()
{
    if (selectedSources.isEmpty()) return;

    QString showId = showTransition->currentData().toString();
    int showDur = showDuration->value();
    QString hideId = hideTransition->currentData().toString();
    int hideDur = hideDuration->value();

    for (auto *src : selectedSources)
        applyTransitionToSource(src, showId, showDur, hideId, hideDur);
}
