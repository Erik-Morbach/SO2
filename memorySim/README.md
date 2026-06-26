# Memory Simulator

Simulador de alocador de memória para trabalho do Grau B da disciplina SO2;

## Arquitetura

![alt text](image.png)

### Componentes

| Componente | Descrição |
|------------|-----------|
| **MMU** | Gerencia a tradução de endereços virtuais para físicos. Possui um `MainMemory` compartilhado (via `shared_ptr`), uma `PageTable` por thread e uma `TLB` compartilhada entre threads. |
| **PageTable** | Implementada como `unordered_set<PageEntry>`, indexada por `std::thread::id`. Mapeia endereço virtual → endereço físico de forma isolada por thread. |
| **PageEntry** | Struct que armazena um par (vAddr, pAddr). Possui hash especializado para `unordered_set` (hash apenas por vAddr). |
| **TLB** | Cache de tradução com tamanho fixo, política de evicção LRU via `FastSegTree` (min-heap baseado em timer). Entradas chaveadas por `(thread_id, pageNumber)`. |
| **MainMemory** | Array 2D `char[frame][offset]`. Bitset para controle de frames alocados. Alocação contígua first-fit. `allocSize[frame][0]` armazena o tamanho original para `free` dinâmico. |

### Segurança entre Threads

- **MMU**: `std::mutex` em todos os métodos públicos (`allocate`, `read`, `write`, `free`)
- **MainMemory**: `std::mutex` em `allocate`, `writeInto`, `readInto`, `free`
- **PageTable**: instância separada por thread via `std::unordered_map<std::thread::id, PageTable>` — sem necessidade de lock entre threads
- **TLB**: entradas chaveadas por `(thread_id, pageNumber)` — threads diferentes não conflitam no cache

### Métricas da TLB

A TLB coleta três métricas automaticamente:

| Métrica | Onde é incrementada | Significado |
|---------|---------------------|-------------|
| **Hits** | `exist()` retorna `true` | Tradução resolvida diretamente na TLB (rápido) |
| **Misses** | `exist()` retorna `false` | Tradução não encontrada na TLB — busca na PageTable |
| **Evictions** | `removeOldest()` | Entrada LRU removida para abrir espaço para uma nova |

### Page Faults

O contador de page faults é incrementado em **duas situações**:

| Situação | Onde é incrementado | Causa |
|----------|---------------------|-------|
| **Capacidade** | `MainMemory::allocate()` | Memória física cheia — não há frames contíguos disponíveis |
| **Página ausente** | `MMU::transform()` | Endereço virtual não possui entrada na tabela de páginas (acesso a região não mapeada) |

Page faults de capacidade ocorrem quando `allocate` não encontra espaço. Page faults de página ausente ocorrem quando `read()` ou `write()` acessam um vAddr que nunca foi alocado ou já foi liberado.

## Configuração

Todas as dimensões são parâmetros de template. Edite `src/main.cpp` para alterar os valores:

| Parâmetro | Exemplo (teste leve) | Exemplo (teste pesado) | Descrição |
|-----------|----------------------|------------------------|-----------|
| `TOTAL_MEM` | `1000` | `65536` (64 KB) | Memória física total em bytes |
| `FRAME_SIZE` | `10` | `8192` (8 KB) | Tamanho do frame/página em bytes |
| `TOTAL_VMEM` | `1000` | `1048576` (1 MB) | Espaço total de endereçamento virtual |
| `TLB_ENTRIES` | `4` | `20` | Número de entradas da TLB (LRU) |

Exemplo de instanciação:

```cpp
// Teste leve
auto mem1 = std::make_shared<MainMemory<1000, 10>>();
auto mmu1 = std::make_shared<MMU<1000, 10, 1000, 4>>(mem1);

// Teste pesado (64 KB físicos, 1 MB virtual, páginas de 8 KB, TLB de 20)
auto mem2 = std::make_shared<MainMemory<65536, 8192>>();
auto mmu2 = std::make_shared<MMU<65536, 8192, 1048576, 20>>(mem2);
```

## Compilar & Executar

Requer CMake e um compilador com suporte a C++17.

```bash
cmake -B build
cmake --build build
./build/MemorySim
```

## Resultados dos Testes

A suíte de testes cobre desde operações unitárias básicas até estresse multi-thread de 10 segundos. Abaixo, a saída completa de cada teste com explicações.

---

### 1. MainMemory — Teste Unitário

**Cenário:** Aloca 15 bytes em uma memória de 100 bytes com frames de 10 bytes (10 frames). Escreve "Testing" e lê a partir do offset 2.

```
Testing Main Memory:
=== MainMemory Summary ===
  TOTAL_MEM: 100 | FRAME_SIZE: 10 | QNT_FRAMES: 10
  Used frames: 0 / 10 (0.0%)
  Page faults: 0
=== MainMemory Summary ===
  TOTAL_MEM: 100 | FRAME_SIZE: 10 | QNT_FRAMES: 10
  Used frames: 2 / 10 (20.0%)
  Page faults: 0
Writing: |Testing| at 0,0
Readed: |sting| from 0,2
```

**Explicação:** Alocar 15 bytes consome 2 frames. A leitura a partir do offset 2 retorna "sting" (pula "Te").

---

### 2. PageTable — Teste Unitário

**Cenário:** Cria tabela com 100 páginas de 10 bytes. Busca entrada inexistente, depois insere e busca.

```
=== PageTable Summary ===
  TOTAL_VMEM=1000 | PAGE_SIZE=10 | PAGES=100
  Entries: 0
Searching for 123: 18446744073709551615, 18446744073709551615
=== PageTable Summary ===
  TOTAL_VMEM=1000 | PAGE_SIZE=10 | PAGES=100
  Entries: 1
    vAddr=12 pAddr=45
Searching for 12: 12, 45
```

**Explicação:** Entrada inexistente retorna `(-1, -1)` (max `size_t`). Após `createNew(12, 45)`, a busca retorna os valores corretos. O hash é apenas por vAddr, ja que nunca teremos mais de uma alocação por endereço virtual para a mesma thread.

---

### 3. TLB — Teste Unitário (LRU)

**Cenário:** TLB de 4 entradas. Preenche até o limite, testa `exist`/`get`, depois força evicção LRU.

```
=== Testing TLB ===
-- Empty TLB --
=== TLB Summary (size=4) ===
  Entries: 0 / 4
  Hits: 0 | Misses: 0 | Evictions: 0

-- Filling TLB (4 entries) --
=== TLB Summary (size=4) ===
  Entries: 4 / 4
  Hits: 0 | Misses: 0 | Evictions: 0
    [0] vAddr=4096 pAddr=40960
    [1] vAddr=8192 pAddr=45056
    [2] vAddr=12288 pAddr=49152
    [3] vAddr=16384 pAddr=53248
  isFull: 1 (expected 1)

-- exist/get --
  exist(4096): 1 (expected 1)
  exist(39321): 0 (expected 0)
  get(8192): vAddr=8192 pAddr=45056 (expected 8192 45056)
  get(39321) on miss: vAddr=18446744073709551615 pAddr=18446744073709551615 (expected -1 -1)

-- Adding past capacity (eviction) --
  Order by timer: 4096(t1), 8192(t2->t5 on get), 12288(t3), 16384(t4)
  Oldest untouched is 4096 (timer=1)
=== TLB Summary (size=4) ===
  Entries: 4 / 4
  Hits: 1 | Misses: 1 | Evictions: 1
    [0] vAddr=20480 pAddr=57344
    [1] vAddr=8192 pAddr=45056
    [2] vAddr=12288 pAddr=49152
    [3] vAddr=16384 pAddr=53248
  exist(4096): 0 (expected 0 - evicted)
  exist(20480): 1 (expected 1 - newly added)
```

**Explicação:** A TLB começa vazia (0 hits, 0 misses, 0 evicções). Ao adicionar 4 entradas, preenche sem evicção. `exist(4096)` retorna 1 (hit) e `exist(39321)` retorna 0 (miss). `get(8192)` atualiza o timer para 5 (refresh). Ao adicionar uma 5ª entrada (20480), a entrada mais antiga (4096, timer=1) é evicída. Métricas finais: 1 hit, 1 miss, 1 evicção.

---

### 4. TLB Full — Evicções em Cadeia

**Cenário:** TLB de 3 entradas. Testa evicções sucessivas, refresh por `get()`, e legibilidade após evicção.

```
=== Testing TLB Full Behavior ===
-- Fill TLB(3) to capacity --
  isFull: 1 (expected 1)
  exist(256): 1 exist(512): 1 exist(768): 1

-- Evict oldest (e1) by adding e4 --
  exist(256): 0 (expected 0 - evicted)
  exist(1024): 1 (expected 1 - new)
  exist(512): 1 exist(768): 1 (expected 1 1)

-- Refresh e2, then evict oldest (e3) --
  exist(768): 0 (expected 0 - oldest untouched)
  exist(512): 1 (expected 1 - refreshed)
  exist(1280): 1 (expected 1 - new)

-- Multiple successive evictions --
  after e6 added, exist(e4): 0 (expected 0 - evicted)
  exist(e6): 1 (expected 1)
  exist(e2): 1 exist(e5): 1 (expected 1 1)

-- Verify entries still readable after eviction chain --
  get(e2): pAddr=2 (expected 2)
  get(e5): pAddr=5 (expected 5)
  get(e6): pAddr=6 (expected 6)
=== TLB Summary (size=3) ===
  Entries: 3 / 3
  Hits: 11 | Misses: 3 | Evictions: 3
    [0] vAddr=1536 pAddr=6
    [1] vAddr=512 pAddr=2
    [2] vAddr=1280 pAddr=5
```

**Explicação:** Testa a política LRU: (1) e1 é o mais antigo → evicído primeiro; (2) `get(e2)` refreshed → e3 se torna o mais antigo → evicído; (3) após múltiplas evicções, as entradas sobreviventes ainda são legíveis com pAddr corretos. 3 evicções, 11 hits, 3 misses totais.

---

### 5. MMU — Integração Completa

**Cenário:** MMU com 4 entradas de TLB. Aloca, escreve, lê, acessa endereço não mapeado, aloca multi-página, libera e realoca.

```
=== Testing MMU ===
-- Allocate + Write / Read --
  read: ABCDE (expected ABCDE)

-- Read from unmapped address (page fault) --
  read(0x2000): -1 (expected -1)

-- Cross-page allocation (size > FRAME_SIZE) --
  read: Hello from multi-page! (expected Hello from multi-page!)

-- Free and reallocate --
  read after realloc: VWXYZ (expected VWXYZ)

-- Page fault counter --
  page faults: 1 (expected 1 - one from unmapped read at 0x2000)
  after unmapped read: 2 (expected 2 - one more from unmapped read at 0x6000)

-- printSummary --
=== MMU Summary ===
  TOTAL_MEM=1000 FRAME_SIZE=10 TOTAL_VMEM=1000 TLB_ENTRIES=4
  Active page tables: 1
    thread=fe8082422f201a91
=== TLB Summary (size=4) ===
  Entries: 4 / 4
  Hits: 6 | Misses: 2 | Evictions: 1
    [0] thread=fe8082422f201a91 vAddr=2048 pAddr=0
    [1] thread=fe8082422f201a91 vAddr=1228 pAddr=1
    [2] thread=fe8082422f201a91 vAddr=1229 pAddr=2
    [3] thread=fe8082422f201a91 vAddr=1230 pAddr=3
=== MainMemory Summary ===
  TOTAL_MEM: 1000 | FRAME_SIZE: 10 | QNT_FRAMES: 100
  Used frames: 4 / 100 (4.0%)
  Page faults: 2
```

**Explicação:** MMU completa o ciclo allocate→write→read com sucesso. Leitura de endereço não mapeado retorna -1 **e também incrementa o contador de page faults** (a página não está presente na tabela de páginas). Alocação de 25 bytes com frame de 10 ocupa 3 páginas (cross-page) — leitura/escrita funciona através de múltiplos frames. `free` dinâmico (sem parâmetro `size`) consulta `MainMemory::getAllocSize`. Page faults: 2 (ambos de leituras de endereços não mapeados: 0x2000 e 0x6000). TLB: 6 hits, 2 misses, 1 evicção.

---

### 6. MMU Multi-thread — Mesmo vAddr, Tabelas e TLB Isoladas

**Cenário:** Duas threads alocam o mesmo endereço virtual `0x1000`, escrevem dados diferentes e leem. Cada thread tem sua própria PageTable **e entradas isoladas na TLB** (chave composta `{thread_id, pageNumber}`).

```
=== Testing MMU Multi-thread (same vAddr, per-thread pt) ===
  Thread1: read back "Thread1!" from vAddr=0x1000
  Thread2: read back "Thread2!" from vAddr=0x1000
=== MMU Summary ===
  TOTAL_MEM=10000 FRAME_SIZE=10 TOTAL_VMEM=10000 TLB_ENTRIES=4
  Active page tables: 2
    thread=6b377aa35fc0046e
    thread=cd1f9a11fd9d1e05
=== TLB Summary (size=4) ===
  Entries: 2 / 4
  Hits: 4 | Misses: 0 | Evictions: 0
    [0] thread=cd1f9a11fd9d1e05 vAddr=409 pAddr=0
    [1] thread=6b377aa35fc0046e vAddr=409 pAddr=1
```

**Explicação:** Cada entrada da TLB é chaveada por `(thread_id, pageNumber)`. As duas threads usam o mesmo pageNumber=409 (`0x1000 / 10`), mas como os thread_ids são dies, a TLB as trata como entradas distintas. Thread1 lê "Thread1!" do 1, Thread2 lê "Thread2!" do frame 0. A TLB Summary exibe o hash do thread_id de cada entrada.

---

### 7. String — Alocação via MMU

**Cenário:** String de capacidade 50, alocada via MMU usando `(size_t)this` como endereço virtual.

```
=== Testing String with MMU ===
  capacity: 50
  length: 0
  after sets, length: 5
  chars: Hello
  after append, length: 12
  string: "Hello World!"
=== MMU Summary ===
  TOTAL_MEM=1000 FRAME_SIZE=10 TOTAL_VMEM=1000 TLB_ENTRIES=4
  Active page tables: 1
    thread=fe8082422f201a91
=== TLB Summary (size=4) ===
  Entries: 4 / 4
  Hits: 12 | Misses: 2 | Evictions: 3
    [0] vAddr=609810160 pAddr=4
    [1] vAddr=609810156 pAddr=0
    [2] vAddr=609810157 pAddr=1
    [3] vAddr=609810159 pAddr=3
=== MainMemory Summary ===
  TOTAL_MEM: 1000 | FRAME_SIZE: 10 | QNT_FRAMES: 100
  Used frames: 5 / 100 (5.0%)
  Page faults: 0
```

**Explicação:** A String usa `(size_t)this` como endereço virtual base — cada instância de String tem seu próprio range virtual. `write("Hello")` + `append(" World!")` resulta em "Hello World!". A MMU gerencia a alocação/liberação automaticamente no construtor/destrutor. 5 frames em uso no final (4 das páginas da MMU do teste anterior + 1 da String). TLB: 12 hits, 2 misses, 3 evicções.

---

### 8. Estresse Multi-thread — Exaustão de Memória (10 segundos)

**Cenário:** Duas threads competem por 8 frames físicos (64 KB) e 128 páginas virtuais (1 MB), alocando sem liberar até exaurir ambos os recursos. Cada thread tenta alocar 512 bytes em todas as 128 páginas do seu range virtual a cada rodada, escreve um padrão único, e ao final da rodada verifica a integridade dos dados antes de liberar.

| Parâmetro | Valor |
|-----------|-------|
| `TOTAL_MEM` | 65.536 bytes (64 KB) — **8 frames de 8 KB** |
| `FRAME_SIZE` | 8.192 bytes (8 KB) |
| `TOTAL_VMEM` | 1.048.576 bytes (1 MB) — **128 páginas** |
| `TLB_ENTRIES` | **8** entradas LRU |
| Threads | 2 threads paralelas durante **10 segundos** |
| Estratégia | Alocar 512 bytes em cada uma das 128 páginas sem liberar |
| Verificação | `read()` + `strcmp()` com o padrão esperado antes do `free()` |

```
=== Testing 10s Parallel MMU Stress (Memory Exhaustion) ===
  TOTAL_MEM=64 KB (8 frames de 8 KB) | TOTAL_VMEM=1024 KB (128 paginas)


  Thread1: 820581 allocs, 25903643 page faults, 0 data errors
  Thread2: 747049 allocs, 24899095 page faults, 0 data errors
  Total: 1567630 allocs | 50802738 page faults | 0 errors

=== MMU Summary ===
  TOTAL_MEM=65536 FRAME_SIZE=8192 TOTAL_VMEM=1048576 TLB_ENTRIES=8
  Active page tables: 2
    thread=6b377aa35fc0046e
    thread=cd1f9a11fd9d1e05
=== TLB Summary (size=8) ===
  Entries: 8 / 8
  Hits: 2468951 | Misses: 666302 | Evictions: 2233924
    [0] thread=cd1f9a11fd9d1e05 vAddr=134 pAddr=6
    [1] thread=cd1f9a11fd9d1e05 vAddr=133 pAddr=5
    [2] thread=cd1f9a11fd9d1e05 vAddr=131 pAddr=3
    [3] thread=cd1f9a11fd9d1e05 vAddr=129 pAddr=1
    [4] thread=cd1f9a11fd9d1e05 vAddr=132 pAddr=4
    [5] thread=6b377aa35fc0046e vAddr=319 pAddr=0
    [6] thread=cd1f9a11fd9d1e05 vAddr=135 pAddr=7
    [7] thread=cd1f9a11fd9d1e05 vAddr=128 pAddr=0
=== MainMemory Summary ===
  TOTAL_MEM: 65536 | FRAME_SIZE: 8192 | QNT_FRAMES: 8
  Used frames: 7 / 8 (87.5%)
  Page faults: 50802738

  Result: PASS (data intact)
```

#### Análise dos Resultados

| Métrica | Valor | Interpretação |
|---------|-------|---------------|
| **Alocações bem-sucedidas** | 1.567.630 | Cada alocação consumiu 1 frame de 8 KB |
| **Page faults** | **50.802.738** | Memória física completamente exaurida — `MainMemory::allocate` não encontrou frames contíguos |
| **Data errors** | **0** | Nenhuma corrupção — toda leitura retornou o padrão esperado |
| **TLB Hits** | 2.468.951 | Traduções resolvidas na TLB |
| **TLB Misses** | 666.302 | Traduções que precisaram consultar a PageTable |
| **TLB Evicções** | **2.233.924** | LRU removendo entradas para abrir espaço — com 8 entradas e dezenas de vAddrs, a TLB vive cheia |
| **Frames usados (final)** | **7 / 8 (87,5%)** | Memória física quase completamente ocupada |
| **Page faults (final)** | **50.802.738** | Contador global de page faults |
| **Tabelas de página** | 2 ativas | Uma por thread |

#### Taxa de Acerto da TLB

```
TLB Hit Rate = 2.468.951 / (2.468.951 + 666.302) ≈ 78,75%
```

Com apenas **8 entradas** na TLB e 128 páginas virtuais disputadas por 2 threads, a taxa de acerto cai para ~79%. A redução no tamanho da TLB causa uma queda drástica na eficiência — 666K misses contra apenas 9 mil (teste com 20 entradas).

#### Page Faults e Exaustão

A cada rodada, cada thread tenta alocar 128 páginas. Como só existem **8 frames físicos**, no máximo 8 alocações por rodada (entre ambas as threads) conseguem frames. As demais ~120 tentativas por thread resultam em **page faults**. Em 10 segundos, as threads somam **50 milhões de page faults**.

Os 8/8 frames ocupados ao final são esperados: quando `stop` é disparado, as threads podem estar no meio de uma rodada de alocação, e as alocações bem-sucedidas daquela rodada não são liberadas.

#### Integridade

**PASS — 0 erros de dados.** Mesmo com 50M page faults, 2.2M evicções de TLB, e 666K misses, **toda leitura retornou o dado correto**. O teste comprova que:
- O isolamento de PageTable por thread funciona mesmo sob contenção extrema
- A TLB com chaveamento por `(thread_id, pageNumber)` mantem o isolamento entre threads
- O free dinâmico (consulta `getAllocSize` na MainMemory) e a reutilização de frames operam corretamente

---

## Resumo

| Teste | O que valida | Resultado |
|-------|-------------|-----------|
| MainMemory | Alocação first-fit, leitura/escrita com offset | OK |
| PageTable | Inserção, busca, entrada inexistente | OK |
| TLB | LRU, exist/get, evicção individual | OK |
| TLB Full | Evicções em cadeia, refresh, legibilidade pós-evicção | OK |
| MMU | Ciclo completo alloc/write/read/free, cross-page, free dinâmico | OK |
| MMU Multi-thread | Isolamento de PageTable + TLB por thread | OK |
| String | Alocação MMU via `(size_t)this`, append, print | OK |
| **Estresse 10s — Exaustão** | **1,56M allocs, 50M page faults, 73% hit rate, zero corrupção** | **OK** |
