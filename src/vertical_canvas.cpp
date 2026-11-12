#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QMouseEvent>

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <vector>
#include <mutex>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("streamforge-vertical", "en-US")

namespace {
struct VerticalScene {
    obs_source_t *source = nullptr; // Owns the private scene source.
    obs_scene_t *scene = nullptr;
    QString name;
};

QWidget *dock = nullptr;
QComboBox *scenePicker = nullptr;
QComboBox *sourcePicker = nullptr;
QListWidget *sceneSources = nullptr;
QLabel *canvasInfo = nullptr;
class VerticalPreview;
VerticalPreview *verticalPreview = nullptr;
std::vector<VerticalScene> verticalScenes;

void refreshSourcePicker();
void selectSceneItemInList(int64_t itemId);

class VerticalPreview final : public QWidget {
public:
    explicit VerticalPreview(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        setMinimumSize(180, 320);
        setMouseTracking(true);
        setCursor(Qt::CrossCursor);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setStyleSheet("background: #171a20; border: 1px solid #343b46;");
    }

    ~VerticalPreview() override
    {
        destroyDisplay();
        clearScene();
    }

    void setSceneSource(obs_source_t *source)
    {
        std::lock_guard<std::mutex> lock(sourceMutex);
        if (sceneSource == source)
            return;
        selectedItemId = -1;
        dragging = false;
        if (sceneSource) {
            if (sourceShowing)
                obs_source_dec_showing(sceneSource);
            obs_source_release(sceneSource);
        }
        sceneSource = source ? obs_source_get_ref(source) : nullptr;
        sourceShowing = sceneSource && isVisible();
        if (sourceShowing)
            obs_source_inc_showing(sceneSource);
    }

protected:
    QPaintEngine *paintEngine() const override { return nullptr; }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(event);
            return;
        }

        obs_source_t *source = nullptr;
        {
            std::lock_guard<std::mutex> lock(sourceMutex);
            source = sceneSource ? obs_source_get_ref(sceneSource) : nullptr;
        }
        if (!source)
            return;

        const uint32_t canvasWidth = obs_source_get_width(source);
        const uint32_t canvasHeight = obs_source_get_height(source);
        obs_source_release(source);
        if (!canvasWidth || !canvasHeight)
            return;

        const float fit = std::min(static_cast<float>(width()) / canvasWidth,
                                   static_cast<float>(height()) / canvasHeight);
        const float viewWidth = canvasWidth * fit;
        const float viewHeight = canvasHeight * fit;
        const float offsetX = (width() - viewWidth) * 0.5f;
        const float offsetY = (height() - viewHeight) * 0.5f;
        const QPointF mouse = event->position();
        if (mouse.x() < offsetX || mouse.x() > offsetX + viewWidth ||
            mouse.y() < offsetY || mouse.y() > offsetY + viewHeight)
            return;

        const float canvasX = (mouse.x() - offsetX) / fit;
        const float canvasY = (mouse.y() - offsetY) / fit;
        obs_scene_t *scene = nullptr;
        {
            std::lock_guard<std::mutex> lock(sourceMutex);
            if (sceneSource)
                scene = obs_scene_from_source(sceneSource);
        }

        struct HitContext {
            float x;
            float y;
            int64_t hitId = -1;
            int row = -1;
            int index = 0;
        } hit{canvasX, canvasY};

        if (scene) {
            obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *param) {
                auto *ctx = static_cast<HitContext *>(param);
                const int row = ctx->index++;
                if (!obs_sceneitem_visible(item))
                    return true;

                obs_source_t *itemSource = obs_sceneitem_get_source(item);
                if (!itemSource)
                    return true;
                vec2 position{}, boxScale{};
                obs_sceneitem_get_pos(item, &position);
                obs_sceneitem_get_box_scale(item, &boxScale);
                const uint32_t align = obs_sceneitem_get_alignment(item);
                const float left = position.x - ((align & OBS_ALIGN_RIGHT) ? boxScale.x :
                                    ((align & OBS_ALIGN_LEFT) ? 0.0f : boxScale.x * 0.5f));
                const float top = position.y - ((align & OBS_ALIGN_BOTTOM) ? boxScale.y :
                                   ((align & OBS_ALIGN_TOP) ? 0.0f : boxScale.y * 0.5f));
                if (ctx->x >= left && ctx->x <= left + boxScale.x &&
                    ctx->y >= top && ctx->y <= top + boxScale.y) {
                    ctx->hitId = obs_sceneitem_get_id(item);
                    ctx->row = row;
                }
                return true;
            }, &hit);
        }

        selectedItemId = hit.hitId;
        selectSceneItemInList(selectedItemId);
        dragging = selectedItemId >= 0;
        if (dragging) {
            dragStart = QPointF(canvasX, canvasY);
            obs_sceneitem_t *item = obs_scene_find_sceneitem_by_id(scene, selectedItemId);
            if (item)
                obs_sceneitem_get_pos(item, &itemStartPosition);
        }
        update();
        event->accept();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!dragging || !(event->buttons() & Qt::LeftButton)) {
            QWidget::mouseMoveEvent(event);
            return;
        }

        obs_scene_t *scene = nullptr;
        {
            std::lock_guard<std::mutex> lock(sourceMutex);
            if (sceneSource)
                scene = obs_scene_from_source(sceneSource);
        }
        if (!scene)
            return;

        obs_sceneitem_t *item = obs_scene_find_sceneitem_by_id(scene, selectedItemId);
        if (!item)
            return;

        obs_source_t *source = obs_scene_get_source(scene);
        if (!source)
            return;
        const float fit = std::min(static_cast<float>(width()) / 1080.0f,
                                   static_cast<float>(height()) / 1920.0f);
        obs_source_release(source);
        if (fit <= 0.0f)
            return;

        const QPointF mouse = event->position();
        const float canvasX = (mouse.x() - (width() - 1080.0f * fit) * 0.5f) / fit;
        const float canvasY = (mouse.y() - (height() - 1920.0f * fit) * 0.5f) / fit;
        vec2 next = itemStartPosition;
        next.x += canvasX - static_cast<float>(dragStart.x());
        next.y += canvasY - static_cast<float>(dragStart.y());
        obs_sceneitem_set_pos(item, &next);
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            dragging = false;
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        {
            std::lock_guard<std::mutex> lock(sourceMutex);
            if (sceneSource && !sourceShowing) {
                obs_source_inc_showing(sceneSource);
                sourceShowing = true;
            }
        }
        createDisplay();
    }

    void hideEvent(QHideEvent *event) override
    {
        destroyDisplay();
        {
            std::lock_guard<std::mutex> lock(sourceMutex);
            if (sceneSource && sourceShowing) {
                obs_source_dec_showing(sceneSource);
                sourceShowing = false;
            }
        }
        QWidget::hideEvent(event);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if (display)
            obs_display_resize(display, static_cast<uint32_t>(std::max(1, event->size().width())),
                               static_cast<uint32_t>(std::max(1, event->size().height())));
    }

private:
    obs_display_t *display = nullptr;
    obs_source_t *sceneSource = nullptr;
    bool sourceShowing = false;
    std::mutex sourceMutex;
    int64_t selectedItemId = -1;
    bool dragging = false;
    QPointF dragStart;
    vec2 itemStartPosition{};

    static void drawPreview(void *param, uint32_t cx, uint32_t cy)
    {
        auto *self = static_cast<VerticalPreview *>(param);
        obs_source_t *source = nullptr;
        {
            std::lock_guard<std::mutex> lock(self->sourceMutex);
            source = self->sceneSource ? obs_source_get_ref(self->sceneSource) : nullptr;
        }

        if (!source || cx == 0 || cy == 0) {
            if (source)
                obs_source_release(source);
            return;
        }

        const uint32_t sourceWidth = obs_source_get_width(source);
        const uint32_t sourceHeight = obs_source_get_height(source);
        if (sourceWidth > 0 && sourceHeight > 0) {
            const float scale = std::min(static_cast<float>(cx) / sourceWidth,
                                         static_cast<float>(cy) / sourceHeight);
            const uint32_t viewWidth = static_cast<uint32_t>(sourceWidth * scale);
            const uint32_t viewHeight = static_cast<uint32_t>(sourceHeight * scale);
            const int offsetX = static_cast<int>((cx - viewWidth) / 2);
            const int offsetY = static_cast<int>((cy - viewHeight) / 2);

            // A display callback shares the OBS graphics context with other
            // displays. Preserve the caller's viewport/projection state so
            // this secondary canvas cannot corrupt the main OBS preview.
            gs_viewport_push();
            gs_projection_push();
            gs_ortho(0.0f, static_cast<float>(sourceWidth), 0.0f,
                     static_cast<float>(sourceHeight), -100.0f, 100.0f);
            gs_set_viewport(offsetX, offsetY, static_cast<int>(viewWidth),
                            static_cast<int>(viewHeight));
            obs_source_video_render(source);
            gs_projection_pop();
            gs_viewport_pop();
        }
        obs_source_release(source);
    }

    void createDisplay()
    {
#ifdef _WIN32
        if (display || !isVisible())
            return;
        const WId nativeId = winId();
        RECT rect{};
        GetClientRect(reinterpret_cast<HWND>(nativeId), &rect);

        gs_init_data init{};
        init.window.hwnd = reinterpret_cast<HWND>(nativeId);
        init.cx = static_cast<uint32_t>(std::max<LONG>(1, rect.right - rect.left));
        init.cy = static_cast<uint32_t>(std::max<LONG>(1, rect.bottom - rect.top));
        init.num_backbuffers = 2;
        init.format = GS_RGBA;
        init.zsformat = GS_ZS_NONE;
        init.adapter = 0;

        display = obs_display_create(&init, 0x20242A);
        if (display)
            obs_display_add_draw_callback(display, drawPreview, this);
#endif
    }

    void destroyDisplay()
    {
        if (!display)
            return;
        obs_display_remove_draw_callback(display, drawPreview, this);
        obs_display_destroy(display);
        display = nullptr;
    }

    void clearScene()
    {
        std::lock_guard<std::mutex> lock(sourceMutex);
        if (sceneSource) {
            if (sourceShowing)
                obs_source_dec_showing(sceneSource);
            obs_source_release(sceneSource);
            sceneSource = nullptr;
            sourceShowing = false;
        }
    }
};


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

void selectSceneItemInList(int64_t itemId)
{
    if (!sceneSources)
        return;
    int selectedRow = -1;
    struct SelectContext {
        int64_t target;
        int row = 0;
        int found = -1;
    } context{itemId};
    obs_scene_t *scene = currentScene();
    if (scene && itemId >= 0) {
        obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *param) {
            auto *ctx = static_cast<SelectContext *>(param);
            if (obs_sceneitem_get_id(item) == ctx->target) {
                ctx->found = ctx->row;
                return false;
            }
            ++ctx->row;
            return true;
        }, &context);
        selectedRow = context.found;
    }
    sceneSources->setCurrentRow(selectedRow);
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
        if (source) {
            const QString name = QString::fromUtf8(obs_source_get_name(source));
            list->addItem(obs_sceneitem_visible(item) ? name : QString("[Oculta] %1").arg(name));
        }
        return true;
    }, sceneSources);
}

void createVerticalScene()
{
    const int number = static_cast<int>(verticalScenes.size()) + 1;
    const QString name = QString("Vertical %1").arg(number);
    obs_data_t *settings = obs_data_create();
    obs_data_set_bool(settings, "custom_size", true);
    obs_data_set_int(settings, "cx", 1080);
    obs_data_set_int(settings, "cy", 1920);

    obs_source_t *sceneSource =
        obs_source_create_private("scene", name.toUtf8().constData(), settings);
    obs_data_release(settings);
    if (!sceneSource) {
        QMessageBox::warning(dock, "StreamForge Vertical", "Não foi possível criar a cena vertical.");
        return;
    }

    // Scene custom dimensions are restored by the scene source load callback.
    obs_source_load(sceneSource);
    obs_scene_t *scene = obs_scene_from_source(sceneSource);
    if (!scene) {
        obs_source_release(sceneSource);
        QMessageBox::warning(dock, "StreamForge Vertical", "O OBS não retornou uma cena válida.");
        return;
    }

    verticalScenes.push_back({sceneSource, scene, name});
    scenePicker->addItem(name);
    scenePicker->setCurrentIndex(scenePicker->count() - 1);
    if (verticalPreview)
        verticalPreview->setSceneSource(sceneSource);
    refreshSceneSourceList();
}

void deleteVerticalScene()
{
    const int index = scenePicker->currentIndex();
    if (index < 0 || static_cast<size_t>(index) >= verticalScenes.size())
        return;

    obs_source_t *sceneSource = verticalScenes[static_cast<size_t>(index)].source;

    // Detach the preview first: it owns an additional reference and may be
    // showing this source while the scene entry is being removed.
    if (verticalPreview && index == scenePicker->currentIndex())
        verticalPreview->setSceneSource(nullptr);

    verticalScenes.erase(verticalScenes.begin() + index);
    if (sceneSource)
        obs_source_release(sceneSource);
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

obs_sceneitem_t *sceneItemAt(obs_scene_t *scene, int targetIndex)
{
    struct FindContext {
        int target;
        int index = 0;
        obs_sceneitem_t *found = nullptr;
    } context{targetIndex};

    obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *param) {
        auto *ctx = static_cast<FindContext *>(param);
        if (ctx->index++ == ctx->target) {
            ctx->found = item;
            return false;
        }
        return true;
    }, &context);
    return context.found;
}

void duplicateSelectedSource()
{
    obs_scene_t *scene = currentScene();
    const int row = sceneSources ? sceneSources->currentRow() : -1;
    obs_sceneitem_t *original = scene ? sceneItemAt(scene, row) : nullptr;
    if (!original)
        return;

    obs_source_t *source = obs_sceneitem_get_source(original);
    if (!source)
        return;

    obs_sceneitem_t *copy = obs_scene_add(scene, source);
    if (copy) {
        vec2 position, scale;
        obs_sceneitem_get_pos(original, &position);
        obs_sceneitem_get_scale(original, &scale);
        obs_sceneitem_set_pos(copy, &position);
        obs_sceneitem_set_scale(copy, &scale);
        obs_sceneitem_set_alignment(copy, obs_sceneitem_get_alignment(original));
        obs_sceneitem_set_rot(copy, obs_sceneitem_get_rot(original));
        obs_sceneitem_set_visible(copy, obs_sceneitem_visible(original));
    }
    refreshSceneSourceList();
    if (sceneSources)
        sceneSources->setCurrentRow(sceneSources->count() - 1);
}

void toggleSelectedSourceVisibility()
{
    obs_scene_t *scene = currentScene();
    const int row = sceneSources ? sceneSources->currentRow() : -1;
    obs_sceneitem_t *item = scene ? sceneItemAt(scene, row) : nullptr;
    if (!item)
        return;
    obs_sceneitem_set_visible(item, !obs_sceneitem_visible(item));
    refreshSceneSourceList();
    if (sceneSources)
        sceneSources->setCurrentRow(row);
}

void moveSelectedSource(int direction)
{
    obs_scene_t *scene = currentScene();
    const int row = sceneSources ? sceneSources->currentRow() : -1;
    obs_sceneitem_t *item = scene ? sceneItemAt(scene, row) : nullptr;
    if (!item)
        return;
    obs_sceneitem_set_order(item, direction < 0 ? OBS_ORDER_MOVE_UP : OBS_ORDER_MOVE_DOWN);
    refreshSceneSourceList();
    if (sceneSources)
        sceneSources->setCurrentRow(std::max(0, row + direction));
}

void renameVerticalScene()
{
    const int index = scenePicker ? scenePicker->currentIndex() : -1;
    if (index < 0 || static_cast<size_t>(index) >= verticalScenes.size())
        return;
    bool accepted = false;
    const QString name = QInputDialog::getText(
        dock, "Renomear cena vertical", "Nome da cena:",
        QLineEdit::Normal, verticalScenes[static_cast<size_t>(index)].name, &accepted).trimmed();
    if (!accepted || name.isEmpty())
        return;
    obs_source_t *sceneSource = obs_scene_get_source(verticalScenes[static_cast<size_t>(index)].scene);
    if (sceneSource)
        obs_source_set_name(sceneSource, name.toUtf8().constData());
    verticalScenes[static_cast<size_t>(index)].name = name;
    scenePicker->setItemText(index, name);
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

    verticalPreview = new VerticalPreview(dock);
    verticalPreview->setMinimumHeight(360);
    root->addWidget(verticalPreview, 2);

    root->addWidget(new QLabel("Cenas verticais"));
    scenePicker = new QComboBox();
    root->addWidget(scenePicker);

    auto *sceneButtons = new QHBoxLayout();
    auto *newScene = new QPushButton("Nova cena");
    auto *removeScene = new QPushButton("Excluir");
    auto *renameScene = new QPushButton("Renomear");
    sceneButtons->addWidget(newScene);
    sceneButtons->addWidget(renameScene);
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
    auto *sourceActions = new QHBoxLayout();
    auto *duplicateSource = new QPushButton("Duplicar");
    auto *toggleVisibility = new QPushButton("Mostrar/ocultar");
    sourceActions->addWidget(duplicateSource);
    sourceActions->addWidget(toggleVisibility);
    root->addLayout(sourceActions);

    auto *orderActions = new QHBoxLayout();
    auto *moveUp = new QPushButton("Subir camada");
    auto *moveDown = new QPushButton("Descer camada");
    orderActions->addWidget(moveUp);
    orderActions->addWidget(moveDown);
    root->addLayout(orderActions);

    auto *removeSource = new QPushButton("Remover fonte selecionada");
    root->addWidget(removeSource);

    auto *note = new QLabel("Clique em uma fonte na prévia para selecioná-la e arraste para reposicionar. O movimento altera somente a cena vertical.");
    note->setWordWrap(true);
    note->setStyleSheet("color: #9aa4b2; padding-top: 8px;");
    root->addWidget(note);

    QObject::connect(newScene, &QPushButton::clicked, createVerticalScene);
    QObject::connect(removeScene, &QPushButton::clicked, deleteVerticalScene);
    QObject::connect(renameScene, &QPushButton::clicked, renameVerticalScene);
    QObject::connect(addSource, &QPushButton::clicked, addSelectedSource);
    QObject::connect(duplicateSource, &QPushButton::clicked, duplicateSelectedSource);
    QObject::connect(toggleVisibility, &QPushButton::clicked, toggleSelectedSourceVisibility);
    QObject::connect(moveUp, &QPushButton::clicked, [] { moveSelectedSource(-1); });
    QObject::connect(moveDown, &QPushButton::clicked, [] { moveSelectedSource(1); });
    QObject::connect(removeSource, &QPushButton::clicked, removeSelectedSource);
    QObject::connect(scenePicker, qOverload<int>(&QComboBox::currentIndexChanged), [] {
        refreshSceneSourceList();
        const int index = scenePicker->currentIndex();
        if (verticalPreview && index >= 0 && static_cast<size_t>(index) < verticalScenes.size())
            verticalPreview->setSceneSource(verticalScenes[static_cast<size_t>(index)].source);
        else if (verticalPreview)
            verticalPreview->setSceneSource(nullptr);
    });

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
    if (verticalPreview)
        verticalPreview->setSceneSource(nullptr);
    obs_frontend_remove_dock("streamforge.vertical");
    verticalPreview = nullptr;
    for (auto &entry : verticalScenes) {
        if (entry.source)
            obs_source_release(entry.source);
    }
    verticalScenes.clear();
    dock = nullptr;
    blog(LOG_INFO, "[StreamForge Vertical] Plugin descarregado.");
}
