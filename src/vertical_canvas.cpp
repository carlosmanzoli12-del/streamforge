#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <vector>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("streamforge-vertical", "en-US")

namespace {
struct VerticalScene {
    obs_scene_t *scene = nullptr;
    QString name;
};

QWidget *dock = nullptr;
QComboBox *scenePicker = nullptr;
QComboBox *sourcePicker = nullptr;
QListWidget *sceneSources = nullptr;
QLabel *canvasInfo = nullptr;
std::vector<VerticalScene> verticalScenes;

void refreshSourcePicker();

void frontendEventCallback(obs_frontend_event event, void *)
{
    if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED ||
        event == OBS_FRONTEND_EVENT_SCENE_CHANGED)
        refreshSourcePicker();
}

obs_scene_t *currentScene()
{
    const int index = scenePicker ? scenePicker->currentIndex() : -1;
    if (index < 0 || static_cast<size_t>(index) >= verticalScenes.size())
        return nullptr;
    return verticalScenes[static_cast<size_t>(index)].scene;
}

bool enumSourceNames(void *data, obs_source_t *source)
{
    auto *combo = static_cast<QComboBox *>(data);
    const uint32_t flags = obs_source_get_output_flags(source);
    if ((flags & OBS_SOURCE_VIDEO) != 0)
        combo->addItem(QString::fromUtf8(obs_source_get_name(source)));
    return true;
}

void refreshSourcePicker()
{
    if (!sourcePicker)
        return;
    const QString previous = sourcePicker->currentText();
    sourcePicker->clear();
    obs_enum_sources(enumSourceNames, sourcePicker);
    const int restore = sourcePicker->findText(previous);
    if (restore >= 0)
        sourcePicker->setCurrentIndex(restore);
}

void refreshSceneSourceList()
{
    if (!sceneSources)
        return;
    sceneSources->clear();
    obs_scene_t *scene = currentScene();
    if (!scene)
        return;

    obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *param) {
        auto *list = static_cast<QListWidget *>(param);
        obs_source_t *source = obs_sceneitem_get_source(item);
        if (source)
            list->addItem(QString::fromUtf8(obs_source_get_name(source)));
        return true;
    }, sceneSources);
}

void createVerticalScene()
{
    const int number = static_cast<int>(verticalScenes.size()) + 1;
    const QString name = QString("Vertical %1").arg(number);
    obs_scene_t *scene = obs_scene_create(name.toUtf8().constData());
    if (!scene) {
        QMessageBox::warning(dock, "StreamForge Vertical", "Não foi possível criar a cena vertical.");
        return;
    }

    verticalScenes.push_back({scene, name});
    scenePicker->addItem(name);
    scenePicker->setCurrentIndex(scenePicker->count() - 1);
    refreshSceneSourceList();
}

void deleteVerticalScene()
{
    const int index = scenePicker->currentIndex();
    if (index < 0 || static_cast<size_t>(index) >= verticalScenes.size())
        return;

    obs_scene_t *scene = verticalScenes[static_cast<size_t>(index)].scene;
    if (scene) {
        obs_source_t *sceneSource = obs_scene_get_source(scene);
        if (sceneSource)
            obs_source_remove(sceneSource);
        obs_scene_release(scene);
    }
    verticalScenes.erase(verticalScenes.begin() + index);
    scenePicker->removeItem(index);
    refreshSceneSourceList();
}

struct RemoveItemContext {
    int targetIndex = -1;
    int currentIndex = 0;
};

bool removeSceneItemAtIndex(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
    auto *context = static_cast<RemoveItemContext *>(param);
    if (context->currentIndex++ == context->targetIndex) {
        obs_sceneitem_remove(item);
        return false;
    }
    return true;
}

void removeSelectedSource()
{
    obs_scene_t *scene = currentScene();
    const int row = sceneSources ? sceneSources->currentRow() : -1;
    if (!scene || row < 0)
        return;

    RemoveItemContext context;
    context.targetIndex = row;
    obs_scene_enum_items(scene, removeSceneItemAtIndex, &context);
    refreshSceneSourceList();
}

void addSelectedSource()
{
    obs_scene_t *scene = currentScene();
    if (!scene || !sourcePicker || sourcePicker->currentText().isEmpty())
        return;

    const QByteArray sourceName = sourcePicker->currentText().toUtf8();
    obs_source_t *source = obs_get_source_by_name(sourceName.constData());
    if (!source)
        return;

    obs_sceneitem_t *item = obs_scene_add(scene, source);
    if (item) {
        vec2 position = {0.0f, 0.0f};
        vec2 scale = {1.0f, 1.0f};
        const uint32_t width = obs_source_get_width(source);
        const uint32_t height = obs_source_get_height(source);

        if (width > 0 && height > 0) {
            const float fit = std::min(1080.0f / static_cast<float>(width),
                                       1920.0f / static_cast<float>(height));
            scale.x = fit;
            scale.y = fit;
            position.x = (1080.0f - static_cast<float>(width) * fit) * 0.5f;
            position.y = (1920.0f - static_cast<float>(height) * fit) * 0.5f;
        }

        obs_sceneitem_set_scale(item, &scale);
        obs_sceneitem_set_pos(item, &position);
    }

    obs_source_release(source);
    refreshSceneSourceList();
}

void buildDock()
{
    dock = new QWidget();
    dock->setObjectName("StreamForgeVerticalDock");
    dock->setMinimumWidth(300);

    auto *root = new QVBoxLayout(dock);
    auto *heading = new QLabel("STREAMFORGE  /  VERTICAL");
    heading->setStyleSheet("font-size: 16px; font-weight: 700; letter-spacing: 1px;");
    root->addWidget(heading);

    canvasInfo = new QLabel("Canvas: 1080 × 1920  •  Proporção 9:16");
    canvasInfo->setStyleSheet("color: #9aa4b2; padding: 4px 0;");
    root->addWidget(canvasInfo);

    root->addWidget(new QLabel("Cenas verticais"));
    scenePicker = new QComboBox();
    root->addWidget(scenePicker);

    auto *sceneButtons = new QHBoxLayout();
    auto *newScene = new QPushButton("Nova cena");
    auto *removeScene = new QPushButton("Excluir");
    sceneButtons->addWidget(newScene);
    sceneButtons->addWidget(removeScene);
    root->addLayout(sceneButtons);

    root->addWidget(new QLabel("Fontes do OBS"));
    sourcePicker = new QComboBox();
    root->addWidget(sourcePicker);
    auto *addSource = new QPushButton("Adicionar fonte à cena vertical");
    root->addWidget(addSource);

    root->addWidget(new QLabel("Fontes na cena"));
    sceneSources = new QListWidget();
    root->addWidget(sceneSources, 1);
    auto *removeSource = new QPushButton("Remover fonte selecionada");
    root->addWidget(removeSource);

    auto *note = new QLabel("As cenas verticais são independentes das cenas horizontais. A edição visual e a pré-visualização vertical serão adicionadas nas próximas etapas.");
    note->setWordWrap(true);
    note->setStyleSheet("color: #9aa4b2; padding-top: 8px;");
    root->addWidget(note);

    QObject::connect(newScene, &QPushButton::clicked, createVerticalScene);
    QObject::connect(removeScene, &QPushButton::clicked, deleteVerticalScene);
    QObject::connect(addSource, &QPushButton::clicked, addSelectedSource);
    QObject::connect(removeSource, &QPushButton::clicked, removeSelectedSource);
    QObject::connect(scenePicker, &QComboBox::currentIndexChanged, refreshSceneSourceList);

    refreshSourcePicker();
    obs_frontend_add_event_callback(frontendEventCallback, nullptr);
}
} // namespace

bool obs_module_load(void)
{
    buildDock();
    if (!obs_frontend_add_dock_by_id("streamforge.vertical", "StreamForge Vertical", dock)) {
        delete dock;
        dock = nullptr;
        return false;
    }
    blog(LOG_INFO, "[StreamForge Vertical] Plugin carregado.");
    return true;
}

void obs_module_unload(void)
{
    obs_frontend_remove_event_callback(frontendEventCallback, nullptr);
    obs_frontend_remove_dock("streamforge.vertical");
    for (auto &entry : verticalScenes) {
        if (entry.scene)
            obs_scene_release(entry.scene);
    }
    verticalScenes.clear();
    dock = nullptr;
    blog(LOG_INFO, "[StreamForge Vertical] Plugin descarregado.");
}
