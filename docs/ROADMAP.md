# Roadmap — StreamForge Vertical

## Etapa 0 — Base nativa
- [x] Repositório dedicado e build CMake inicial.
- [x] Dock nativo registrado no OBS.
- [x] Criar/excluir cenas verticais privadas em memória.
- [x] Criar cenas com dimensões personalizadas 1080 × 1920 usando a configuração de cena do OBS 30.0.2.
- [x] Listar fontes de vídeo existentes no OBS.
- [x] Inserir uma fonte existente em uma cena vertical e aplicar ajuste inicial para 1080 × 1920.
- [x] Renomear cenas verticais.
- [x] Duplicar uma camada preservando posição, escala, rotação e visibilidade.
- [x] Alternar visibilidade e alterar ordem das camadas.
- [ ] Validar compilação no SDK/dependências do OBS 30.0.2.
- [ ] Testar carregamento/descarregamento no OBS 30.0.2 para Windows.

## Etapa 1 — Canvas real
- [x] Implementar widget de pré-visualização 9:16 com obs_display e renderização da cena vertical.
- [x] Gerenciar estado showing da fonte e ciclo de vida do display ao abrir/fechar o dock.
- [ ] Validar renderização e ciclo de vida no OBS 30.0.2 x64 em Windows.
- [ ] Implementar seleção de item diretamente na prévia.
- [ ] Arrastar, redimensionar e girar fontes com alças.
- [ ] Zoom, pan, grade, guias e snapping.
- [ ] Ajustar ao canvas, preencher, centralizar e alinhar.

## Etapa 2 — Gerenciamento de composição
- [x] Remover e reordenar fontes.
- [x] Duplicar uma instância de fonte dentro da cena.
- [x] Renomear cenas.
- [ ] Duplicar cenas completas.
- [x] Visibilidade por fonte.
- [ ] Bloqueio por fonte.
- [ ] Recorte, limites, rotação e transformações avançadas.
- [ ] Seleção múltipla.

## Etapa 3 — Persistência e integração
- [ ] Salvar cenas verticais na coleção de cenas do OBS.
- [ ] Restaurar estado após reiniciar o OBS.
- [ ] Integrar com troca de coleção e fechamento do OBS.
- [ ] Vínculos opcionais horizontal/vertical.
- [ ] Desfazer/refazer e atalhos.

## Etapa 4 — Qualidade e entrega
- [ ] Testes de compatibilidade no OBS 30.0.2 x64 (incluindo preview 9:16).
- [ ] Testes de fontes comuns, mídia, captura de jogo e navegador.
- [ ] Testes de DPI, múltiplos monitores e redimensionamento do dock.
- [ ] Pacote de instalação e instruções de remoção.
- [ ] Release versionada somente após validação real.

**Estado:** protótipo inicial do dock e operações básicas. Não é uma versão funcional completa do Canvas Vertical.
