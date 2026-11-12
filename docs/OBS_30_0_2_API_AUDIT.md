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

A implementação atual em `src/vertical_canvas.cpp` usa um widget Qt com janela nativa (`winId()` no Windows), cria `gs_init_data` com HWND e dimensões do widget, cria `obs_display_t` e registra um callback de desenho. O callback do display é executado no contexto gráfico do OBS. A implementação mantém a fonte em estado *showing* enquanto a prévia está visível, renderiza a cena com `obs_source_video_render()` e usa `gs_viewport_push/pop` e `gs_projection_push/pop` para preservar o estado gráfico compartilhado com os demais displays. No encerramento, remove o callback e destrói o display.

Isso confirma que existe uma implementação de prévia renderizada no código, mas **não confirma que ela funciona corretamente em runtime**. Ainda é necessário compilar e testar no Windows com OBS 30.0.2 x64, inclusive abertura/fechamento repetido do dock, resize, troca de cenas, fontes de navegador/mídia e encerramento do OBS.

## Descoberta sobre dimensões independentes

O código de `libobs/obs-scene.c` na tag 30.0.2 confirma que cenas suportam `custom_size`, `cx` e `cy` em suas configurações. `scene_load()` lê esses campos e `scene_getwidth()` / `scene_getheight()` retornam as dimensões personalizadas quando `custom_size` está ativo. O protótipo agora cria uma fonte privada do tipo `scene` com `custom_size=true`, `cx=1080`, `cy=1920`, chama `obs_source_load()` para aplicar a configuração e obtém o `obs_scene_t` com `obs_scene_from_source()`.

Isso é a base implementada para coordenadas independentes 9:16. A criação privada, a aplicação da configuração e o ciclo de vida da fonte ainda precisam ser validados em runtime no OBS 30.0.2 x64.

## Próximos passos técnicos

1. Compilar e validar o widget de display Qt/OBS e o ciclo de vida de `obs_display_t` em Windows.
2. Validar o motor de composição vertical independente baseado em cenas OBS com `custom_size` 1080×1920 (dimensões, coordenadas, recorte e transformações).
3. Conectar seleção/arraste/redimensionamento ao modelo de transformação, sem modificar a cena horizontal original.
4. Persistir o modelo vertical por coleção de cenas.
5. Compilar e testar no OBS 30.0.2 x64 antes de publicar instalador.

## Limites desta auditoria

- Headers verificados na tag de código-fonte 30.0.2.
- Não houve compilação do plugin nesta execução.
- Não houve teste de carregamento dentro do OBS.
- A API de frontend deve ser validada com o pacote de desenvolvimento do OBS 30.0.2 no Windows antes da build final.


## Edição direta inicial no canvas

A branch também implementa interação de mouse no widget: clique para selecionar uma camada por hit-test da caixa transformada e arraste para atualizar sua posição por `obs_sceneitem_set_pos()`. A seleção é refletida na lista de camadas. O hit-test inicial usa caixas alinhadas aos eixos e não representa ainda a geometria exata de itens rotacionados ou com recortes complexos; essa limitação deve ser resolvida antes de declarar a edição visual completa. O comportamento ainda não foi compilado nem validado em runtime.
