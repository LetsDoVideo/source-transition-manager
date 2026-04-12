#include "SourceTransitionDock.hpp"
#include <obs-module.h>
#include <plugin-support.h>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGroupBox>

static const QList<QPair<QString, QString>> TRANSITION_TYPES = {
    {"cut_transition",           "Cut"},
    {"fade_transition",          "Fade"},
    {"swipe_transition",         "Swipe"},
    {"slide_transition",         "Slide"},
    {"obs_stinger_transition",   "Stinger"},
    {"fade_to_color_transition", "Fade to Color"},
    {"wipe_transition",          "Luma Wipe"},
};

// Signal handler trampoline — OBS signals are C callbacks
static void onItemSelect(void *data, calldata_t *)
{
    auto *dock = static_cast<SourceTransitionDock *>(data);
    QMetaObject::invokeMethod(dock, "refreshSelectedSources", Qt::QueuedConnection);
}

SourceTransitionDock::SourceTransitionDock(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    obs_frontend_add_event_callback(frontendEventCallback, this);
}

SourceTransitionDock::~SourceTransitionDock()
{
    obs_frontend_remove_event_callback(frontendEventCallback, this);
    disconnectSceneSignals();
    for (auto *item : selectedItems)
        obs_sceneitem_release(item);
}

void SourceTransitionDock::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    placeholderLabel = new QLabel("Select a source to see transitions", this);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setWordWrap(true);
    placeholderLabel->setStyleSheet("color: gray; padding: 12px;");
    mainLayout->addWidget(placeholderLabel);

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

    QHBoxLayout *showTypeRow = new QHBoxLayout();
    showTypeRow->addWidget(new QLabel("Type:", showGroup));
    showTypeRow->addWidget(showTransition, 1);
    showLayout->addLayout(showTypeRow);

    QHBoxLayout *showDurRow = new QHBoxLayout();
    showDurRow->addWidget(new QLabel("Duration:", showGroup));
    showDurRow->addWidget(showDuration, 1);
    showLayout->addLayout(showDurRow);

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

    QHBoxLayout *hideTypeRow = new QHBoxLayout();
    hideTypeRow->addWidget(new QLabel("Type:", hideGroup));
    hideTypeRow->addWidget(hideTransition, 1);
    hideLayout->addLayout(hideTypeRow);

    QHBoxLayout *hideDurRow = new QHBoxLayout();
    hideDurRow->addWidget(new QLabel("Duration:", hideGroup));
    hideDurRow->addWidget(hideDuration, 1);
    hideLayout->addLayout(hideDurRow);

    controlsLayout->addWidget(hideGroup);

    applyAllButton = new QPushButton("Apply to All Selected Sources", controlsWidget);
    controlsLayout->addWidget(applyAllButton);
    controlsLayout->addStretch();

    mainLayout->addWidget(controlsWidget);
    controlsWidget->hide();

    connect(showTransition, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SourceTransitionDock::onShowTransitionChanged);
    connect(hideTransition, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SourceTransitionDock::onHideTransitionChanged);
    connect(applyAllButton, &QPushButton::clicked,
            this, &SourceTransitionDock::onApplyToAll);
}

void SourceTransitionDock::connectSceneSignals(obs_source_t *scene_source)
{
    if (!scene_source) return;
    signal_handler_t *sh = obs_source_get_signal_handler(scene_source);
    if (!sh) return;
    signal_handler_connect(sh, "item_select",   onItemSelect, this);
    signal_handler_connect(sh, "item_deselect", onItemSelect, this);
    currentScene = scene_source;
}

void SourceTransitionDock::disconnectSceneSignals()
{
    if (!currentScene) return;
    signal_handler_t *sh = obs_source_get_signal_handler(currentScene);
    if (sh) {
        signal_handler_disconnect(sh, "item_select",   onItemSelect, this);
        signal_handler_disconnect(sh, "item_deselect", onItemSelect, this);
    }
    currentScene = nullptr;
}

void SourceTransitionDock::frontendEventCallback(obs_frontend_event event, void *data)
{
    auto *dock = static_cast<SourceTransitionDock *>(data);
    switch (event) {
    case OBS_FRONTEND_EVENT_SCENE_CHANGED:
    case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
        QMetaObject::invokeMethod(dock, "onSceneChanged", Qt::QueuedConnection);
        break;
    default:
        break;
    }
}

void SourceTransitionDock::onSceneChanged()
{
    disconnectSceneSignals();

    obs_source_t *scene_source = obs_frontend_get_current_scene();
    if (!scene_source) {
        placeholderLabel->show();
        controlsWidget->hide();
        return;
    }

    connectSceneSignals(scene_source);
    obs_source_release(scene_source);

    refreshSelectedSources();
}

void SourceTransitionDock::refreshSelectedSources()
{
    for (auto *item : selectedItems)
        obs_sceneitem_release(item);
    selectedItems.clear();

    obs_source_t *scene_source = obs_frontend_get_current_scene();
    if (!scene_source) {
        placeholderLabel->show();
        controlsWidget->hide();
        return;
    }

    obs_scene_t *scene = obs_scene_from_source(scene_source);
    obs_source_release(scene_source);

    obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *data) -> bool {
        if (obs_sceneitem_selected(item)) {
            auto *dock = static_cast<SourceTransitionDock *>(data);
            obs_sceneitem_addref(item);
            dock->selectedItems.append(item);
        }
        return true;
    }, this);

    if (selectedItems.isEmpty()) {
        placeholderLabel->show();
        controlsWidget->hide();
        return;
    }

    placeholderLabel->hide();
    controlsWidget->show();
    loadTransitionsForItem(selectedItems.first());
}

void SourceTransitionDock::loadTransitionsForItem(obs_sceneitem_t *item)
{
    showTransition->blockSignals(true);
    hideTransition->blockSignals(true);
    showDuration->blockSignals(true);
    hideDuration->blockSignals(true);

    // Do NOT release — we don't own this reference
    obs_source_t *showTr = obs_sceneitem_get_transition(item, true);
    if (showTr) {
        const char *id = obs_source_get_id(showTr);
        int idx = showTransition->findData(QString(id));
        if (idx >= 0) showTransition->setCurrentIndex(idx);
    } else {
        showTransition->setCurrentIndex(0);
    }
    showDuration->setValue((int)obs_sceneitem_get_transition_duration(item, true));

    // Do NOT release — we don't own this reference
    obs_source_t *hideTr = obs_sceneitem_get_transition(item, false);
    if (hideTr) {
        const char *id = obs_source_get_id(hideTr);
        int idx = hideTransition->findData(QString(id));
        if (idx >= 0) hideTransition->setCurrentIndex(idx);
    } else {
        hideTransition->setCurrentIndex(0);
    }
    hideDuration->setValue((int)obs_sceneitem_get_transition_duration(item, false));

    showTransition->blockSignals(false);
    hideTransition->blockSignals(false);
    showDuration->blockSignals(false);
    hideDuration->blockSignals(false);
}

void SourceTransitionDock::applyTransitionToItem(obs_sceneitem_t *item,
    const QString &showId, int showDur,
    const QString &hideId, int hideDur)
{
    obs_source_t *showTr = obs_source_create_private(
        showId.toUtf8().constData(), nullptr, nullptr);
    if (showTr) {
        obs_sceneitem_set_transition(item, true, showTr);
        obs_sceneitem_set_transition_duration(item, true, (uint32_t)showDur);
        obs_source_release(showTr);
    }

    obs_source_t *hideTr = obs_source_create_private(
        hideId.toUtf8().constData(), nullptr, nullptr);
    if (hideTr) {
        obs_sceneitem_set_transition(item, false, hideTr);
        obs_sceneitem_set_transition_duration(item, false, (uint32_t)hideDur);
        obs_source_release(hideTr);
    }
}

void SourceTransitionDock::onShowTransitionChanged()
{
    if (selectedItems.isEmpty()) return;
    QString id = showTransition->currentData().toString();
    int dur = showDuration->value();
    for (auto *item : selectedItems) {
        obs_source_t *tr = obs_source_create_private(
            id.toUtf8().constData(), nullptr, nullptr);
        if (tr) {
            obs_sceneitem_set_transition(item, true, tr);
            obs_sceneitem_set_transition_duration(item, true, (uint32_t)dur);
            obs_source_release(tr);
        }
    }
}

void SourceTransitionDock::onHideTransitionChanged()
{
    if (selectedItems.isEmpty()) return;
    QString id = hideTransition->currentData().toString();
    int dur = hideDuration->value();
    for (auto *item : selectedItems) {
        obs_source_t *tr = obs_source_create_private(
            id.toUtf8().constData(), nullptr, nullptr);
        if (tr) {
            obs_sceneitem_set_transition(item, false, tr);
            obs_sceneitem_set_transition_duration(item, false, (uint32_t)dur);
            obs_source_release(tr);
        }
    }
}

void SourceTransitionDock::onApplyToAll()
{
    if (selectedItems.isEmpty()) return;
    QString showId = showTransition->currentData().toString();
    int showDur = showDuration->value();
    QString hideId = hideTransition->currentData().toString();
    int hideDur = hideDuration->value();
    for (auto *item : selectedItems)
        applyTransitionToItem(item, showId, showDur, hideId, hideDur);
}
