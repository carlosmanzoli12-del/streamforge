# Produto: StreamForge Vertical

## Objetivo

Construir um plugin nativo para OBS Studio 30.0.2 que ofereça um fluxo de trabalho de produção vertical 9:16, com cenas e composição independentes das cenas horizontais. O produto é inspirado nas capacidades públicas do Canvas Vertical do SE.Live, mas terá implementação e identidade próprias.

## Incluído

1. Canvas configurável em proporção retrato, com 1080 × 1920 como predefinição.
2. Coleção de cenas verticais independente da coleção de cenas horizontal.
3. Reutilização de fontes existentes do OBS.
4. Pré-visualização vertical renderizada.
5. Seleção, inclusão, remoção, reordenação e duplicação de fontes.
6. Transformação visual: posição, escala, rotação, recorte, limites e alinhamento.
7. Operações de seleção múltipla e bloqueio/visibilidade de fontes.
8. Duplicar, renomear, ordenar e excluir cenas verticais.
9. Persistência por coleção de cenas e restauração ao reabrir o OBS.
10. Vínculo opcional entre cena horizontal e cena vertical, sem tornar uma dependente da outra.
11. Atalhos, desfazer/refazer e validação de comportamento em diferentes resoluções/DPI.

## Fora de escopo

- Multistream, destinos de transmissão e gerenciamento de chaves.
- OAuth, login de plataformas, Twitch ou TikTok.
- Chat unificado, alertas, widgets e overlays de plataformas.
- Recursos não relacionados à composição vertical.

## Critério de conclusão

Não considerar pronto até que o plugin compile com as dependências compatíveis com OBS 30.0.2, carregue no Windows, crie e restaure cenas verticais, renderize a prévia corretamente e permita editar fontes com transformações persistentes. Cada operação deverá ser testada em OBS real; um dock que apenas exibe controles não atende ao objetivo.
