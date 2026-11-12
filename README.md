# StreamForge Vertical

Plugin nativo para OBS Studio 30.0.2, dedicado exclusivamente à criação e edição de cenas verticais (9:16), inspirado no fluxo de trabalho do Canvas Vertical do SE.Live.

## Escopo

- Canvas vertical independente, inicialmente em 1080 × 1920.
- Cenas verticais independentes das cenas horizontais.
- Adição de fontes existentes do OBS às cenas verticais.
- Organização básica de cenas e fontes.
- Dock próprio integrado à interface do OBS.

Este repositório é uma implementação independente. Não contém código proprietário do SE.Live.

## Estado atual

A primeira etapa cria a estrutura do plugin, o dock do OBS e operações iniciais de gerenciamento de cenas/fontes. Ainda não representa uma versão final: pré-visualização renderizada, edição visual por arrastar, transformações avançadas, persistência e validação no SDK exato do OBS 30.0.2 precisam ser concluídas e testadas.

## Requisitos

- Windows 10/11 x64
- OBS Studio 30.0.2 (SDK/dependências de desenvolvimento correspondentes)
- CMake 3.24+
- Visual Studio 2022 com C++ Desktop
- Qt 6 compatível com a compilação do OBS

## Compilação

Configure com os caminhos de instalação do SDK/dependências do OBS:

```powershell
cmake -S . -B build -A x64 -DSTREAMFORGE_BUILD_OBS_PLUGIN=ON
cmake --build build --config Release
```

Os nomes dos pacotes CMake e caminhos variam conforme o pacote de desenvolvimento utilizado. A compilação só será considerada validada após build e teste de carregamento no OBS 30.0.2.
