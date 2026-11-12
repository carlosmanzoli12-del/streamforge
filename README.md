# StreamForge Vertical

Plugin nativo para OBS Studio 30.0.2, dedicado exclusivamente à criação e edição de cenas verticais (9:16), inspirado no fluxo de trabalho do Canvas Vertical do SE.Live.

## Escopo

- Canvas vertical independente, inicialmente em 1080 × 1920.
- Cenas verticais independentes das cenas horizontais.
- Adição de fontes existentes do OBS às cenas verticais.
- Renomear cenas verticais.
- Duplicar instâncias de fontes dentro da cena.
- Mostrar/ocultar fontes e reorganizar camadas.
- Remover fontes e cenas verticais.
- Dock próprio integrado à interface do OBS.

Este repositório é uma implementação independente. Não contém código proprietário do SE.Live.

## Estado atual

A base inclui cenas privadas com dimensões 1080 × 1920, dock nativo, gerenciamento de camadas e uma implementação inicial de pré-visualização renderizada via libobs. O callback da prévia preserva as pilhas de viewport e projeção do contexto gráfico compartilhado. A prévia ainda precisa de validação em execução no OBS 30.0.2 e não inclui seleção/arraste interativo. Ainda não representa uma versão final: edição visual direta, transformações avançadas, persistência e validação no SDK exato do OBS 30.0.2 precisam ser concluídas e testadas.

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


## Gerar o instalador do Windows

O projeto usa CPack com NSIS. Para gerar o instalador `.exe`, são necessários:
- Visual Studio 2022 (C++ x64);
- CMake 3.24 ou superior;
- NSIS instalado e disponível no PATH;
- SDK/dependências de desenvolvimento do OBS 30.0.2, incluindo libobs, obs-frontend-api e Qt 6 compatível.

Exemplo no PowerShell, com o ambiente de dependências do OBS configurado:

```powershell
$env:OBS_SDK_ROOT = "C:/obs-sdk"
.\scripts\build-installer.ps1 -Configuration Release
```

O script verifica CMake, CPack e NSIS, configura o projeto, compila em x64 e gera o instalador. O instalador coloca o módulo em `obs-plugins/64bit`; confirme o diretório de destino no assistente. Feche o OBS antes de instalar ou atualizar o plugin e abra-o novamente após a instalação.

**Nota:** o instalador é gerado a partir do binário compilado. Este repositório ainda não contém um instalador pronto; a compilação e a instalação precisam ser validadas em Windows com o SDK correspondente ao OBS 30.0.2.
