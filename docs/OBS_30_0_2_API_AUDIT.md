# Auditoria das APIs do OBS Studio 30.0.2

Esta auditoria usa os headers da tag `30.0.2` do repositório oficial `obsproject/obs-studio`. Ela não substitui uma compilação contra o SDK binário do Windows.

## APIs confirmadas na tag 30.0.2

Arquivo `libobs/obs.h`:

- `obs_display_create(const struct gs_init_data *, uint32_t background_color)`
- `obs_display_destroy(obs_display_t *)`
- `obs_display_add_draw_callback(...)`
- `obs_display_remove_draw_callback(...)`
- `obs_source_video_render(obs_source_t *)`
- `obs_source_inc_showing(obs_source_t *)` e `obs_source_dec_showing(obs_source_t *)`
- `obs_sceneitem_get_crop(...)` e `obs_sceneitem_set_crop(...)`
- `obs_render_main_texture()`

O cabeçalho confirma que a criação de display recebe **dois argumentos** na versão 30.0.2. Não copiar exemplos de versões recentes sem conferir a assinatura.

## Arquitetura de pré-visualização

A prévia dentro do dock deve usar um widget Qt com janela nativa (`winId()` no Windows), criar um `gs_init_data` com o HWND e dimensões do widget, criar `obs_display_t`, e registrar um callback de desenho. O callback é executado no contexto gráfico do OBS. Ao desenhar uma cena específica, manter o source em estado *showing* enquanto a prévia estiver ativa e renderizá-lo com `obs_source_video_render()`; remover callback e destruir o display antes de destruir o widget.

## Descoberta sobre dimensões independentes

O código de `libobs/obs-scene.c` na tag 30.0.2 confirma que cenas suportam `custom_size`, `cx` e `cy` em suas configurações. `scene_load()` lê esses campos e `scene_getwidth()` / `scene_getheight()` retornam as dimensões personalizadas quando `custom_size` está ativo. O protótipo agora cria uma fonte privada do tipo `scene` com `custom_size=true`, `cx=1080`, `cy=1920`, chama `obs_source_load()` para aplicar a configuração e obtém o `obs_scene_t` com `obs_scene_from_source()`.

Isso é a base correta para coordenadas independentes 9:16 e foi implementado na branch. Ainda precisa ser validado em runtime: a criação privada, a aplicação da configuração e o ciclo de vida da fonte devem ser testados no OBS 30.0.2 x64. Não equivale ainda a uma prévia renderizada.

## Próximos passos técnicos

1. Construir um protótipo de display Qt/OBS e validar o ciclo de vida de `obs_display_t` em Windows.
2. Validar o motor de composição vertical independente baseado em cenas OBS com `custom_size` 1080×1920 (dimensões, coordenadas, recorte e transformações).
3. Conectar seleção/arraste/redimensionamento ao modelo de transformação, sem modificar a cena horizontal original.
4. Persistir o modelo vertical por coleção de cenas.
5. Compilar e testar no OBS 30.0.2 x64 antes de publicar instalador.

## Limites desta auditoria

- Headers verificados na tag de código-fonte 30.0.2.
- Não houve compilação do plugin nesta execução.
- Não houve teste de carregamento dentro do OBS.
- A API de frontend deve ser validada com o pacote de desenvolvimento do OBS 30.0.2 no Windows antes da build final.
