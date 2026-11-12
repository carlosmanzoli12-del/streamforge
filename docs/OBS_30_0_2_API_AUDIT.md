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

## Limitação arquitetural que precisa ser resolvida

Na tag 30.0.2, o caminho de renderização de uma cena OBS tradicional usa as dimensões globais de vídeo da instância. Criar um `obs_scene_t` separado **não cria automaticamente um canvas com dimensões independentes**. Portanto, apenas desenhar essa cena dentro de um retângulo 9:16 não garante composição vertical real: a cena pode continuar tendo as dimensões horizontais do projeto.

Antes de implementar o editor interativo, o motor vertical precisa estabelecer coordenadas e dimensões de composição 1080×1920 independentes e uma forma segura de renderizar as fontes existentes nesse espaço. Não tratar uma prévia recortada/esticada de uma cena horizontal como canvas vertical concluído.

## Próximos passos técnicos

1. Construir um protótipo de display Qt/OBS e validar o ciclo de vida de `obs_display_t` em Windows.
2. Definir e testar o motor de composição vertical independente (dimensões, coordenadas, recorte e transformações).
3. Conectar seleção/arraste/redimensionamento ao modelo de transformação, sem modificar a cena horizontal original.
4. Persistir o modelo vertical por coleção de cenas.
5. Compilar e testar no OBS 30.0.2 x64 antes de publicar instalador.

## Limites desta auditoria

- Headers verificados na tag de código-fonte 30.0.2.
- Não houve compilação do plugin nesta execução.
- Não houve teste de carregamento dentro do OBS.
- A API de frontend deve ser validada com o pacote de desenvolvimento do OBS 30.0.2 no Windows antes da build final.
