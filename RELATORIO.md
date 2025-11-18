# Relatório do Projeto 2: Simulador de Memória Virtual

**Disciplina:** Sistemas Operacionais
**Professor:** Lucas Figueiredo
**Data:**

## Integrantes do Grupo

- Nome Completo - Matrícula
- Nome Completo - Matrícula
- Nome Completo - Matrícula
- Nome Completo - Matrícula

---

## 1. Instruções de Compilação e Execução

### 1.1 Compilação

Descreva EXATAMENTE como compilar seu projeto. Inclua todos os comandos necessários.

gcc -o simulador simulador.c


### 1.2 Execução

Forneça exemplos completos de como executar o simulador.

O simulador aceita três argumentos: o algoritmo (fifo ou clock), o arquivo de configuração e o arquivo de acessos.

./simulador fifo config_exemplo.txt acessos_exemplo.txt


./simulador clock config_exemplo.txt acessos_exemplo.txt

---

## 2. Decisões de Design

### 2.1 Estruturas de Dados

Descreva as estruturas de dados que você escolheu para representar:

**Tabela de Páginas:**
Foi utilizada um struct chamado EntradaPagina armazenada em um array dinamico (EntradaPagina *tabela_paginas) dentro de cada processo e assim, cada um possui sua própria tabela de páginas implementada como um vetor indexado pelo número da página. Cada entrada da tabela contém: 
int frame_fisico;   // Índice do frame físico onde a página está carregada
int presente;        // Indica se a página está na memória física (bit de presença)
int r_bit;           // Bit de referência, usado pelo algoritmo CLOCK
int tempo_carga;     // Momento em que a página foi carregada (usado pelo FIFO)

Para múltiplos processos, o programa define um vetor global de procesos (Processo *processos).
- **Justificativa:** 
Achamos adequada essa abordagem pois o número de páginas de cada processo é conhecido a partir do tamanho do espaço virtual e simulta bem o comportamento de uma tabela de páginas real

**Frames Físicos:**
Os frama das memórias físicas são armazenadas em um vetor global.
FrameFísico *memoria_fisica

Para cada frame é armazenado:
int ocupado;       // Indica se o frame está sendo usado
int pid_dono;      // PID do processo dono da página
int pagina_dono;   // Índice da página que ocupa este frame

O campo ocupado(0 ou 1) indica diretamente se o frame está livre, e a função auxiliar encontrar_frame_livre() percorre o vetor memoria_fisica procurando o primeiro frame com ocupado==0;
- **Justificativa:** 
Ao usar um vetor, facilita a implementação dos algoritmos de substituição, pois permite a indexação direta por número de frame.

**Estrutura para FIFO:**
O algoritmo FIFO utiliza a variável tempo_carga em cada entrada de página para registrar o instante em que ela foi carregada:
entrada_tp->tempo_carga = contador_carga++;
A cada carga, o contador é incrementado.
A função selecionar_vitima_fifo() percorre todos os frames e seleciona o que contém a página com menor valor de tempo_carga, ou seja, a que entrou há mais tempo.

- **Justificativa:** 
Essa abordagem evita a necessidade de uma fila explícita, simplificando a implementação, além do tempo_carga permitir a determinação da ordem de chegada.

**Estrutura para Clock:**
Um ponteiro global ponteiro_clock percorre os frames da memória em ordem circular, e após analisar um frame o ponteiro é atualizado por ponteiro_clock =(ponteiro_clock + 1) % NUM_FRAMES.
O bit de referência r-bit está armazenado em cada entrada da tabela de páginas. Se r_bit==0, a página é escolhida como vítima, se for ==1 é resetado para 0 e o ponteiro avança
- **Justificativa:** 
Escolhemos esta abordagem pois ela reproduz o funcionamento do Clock, onde o ponteiro varre os frames de maneira cíclica.

### 2.2 Organização do Código

Descreva como organizou seu código:

- Quantos arquivos/módulos criou?
- Qual a responsabilidade de cada arquivo/módulo?
- Quais são as principais funções e o que cada uma faz?

simulador.c
├── main()                    - Função principal: gerencia a execução
├── ler_configuracao()        - Lê e inicializa os dados de configuração
├── processar_acessos()       - Lê e processa todos os acessos de memória
├── processar_acesso()        - Realiza o tratamento de um único acesso
│   ├── traduzir_endereco()   - Traduz endereço virtual em página e deslocamento
│   ├── encontrar_frame_livre() - Busca frame disponível
│   ├── selecionar_vitima_fifo() - Seleciona página vítima (FIFO)
│   └── selecionar_vitima_clock() - Seleciona página vítima (Clock)
├── liberar_memoria()         - Libera memória alocada dinamicamente
```

### 2.3 Algoritmo FIFO

Explique **como** implementou a lógica FIFO:

O algoritmo FIFO foi implementado utilizando um contador global que registra o momento em que cada página é carregada na memória. Cada entrada da tabela de páginas possui um campo chamado tempo_carga, que armazena esse valor no instante da alocação. Assim, a ordem de chegada das páginas é mantida de forma implícita, sem necessidade de filas adicionais. Quando ocorre um page fault e não há frames livres, a função percorre todos os frames físicos e identifica a página com o menor valor de tempo_carga, ou seja, a mais antiga na memória. Essa página é escolhida como vítima, removida da memória física, e sua entrada na tabela de páginas é marcada como ausente. Em seguida, a nova página é carregada no mesmo frame, seu tempo de carga é atualizado e o contador global é incrementado para manter a ordem temporal das futuras substituições.

### 2.4 Algoritmo Clock

Explique **como** implementou a lógica Clock:

O algoritmo Clock usa um ponteiro circular que percorre os frames em ordem. Cada página tem um bit de referência (r_bit), que é setado em todo acesso. Ao escolher uma vítima, o algoritmo verifica o frame apontado: se r_bit = 0, a página é substituída; se r_bit = 1, o bit é zerado e o ponteiro avança, dando uma “segunda chance”. Se todas as páginas tiverem r_bit = 1, o ponteiro continua girando até encontrar a primeira com r_bit = 0, garantindo que sempre haja substituição.

### 2.5 Tratamento de Page Fault

Explique como seu código distingue e trata os dois cenários:

**Cenário 1: Frame livre disponível**
O código verifica uma lista de ocupação para identificar se há algum frame livre. Quando encontra um, carrega a nova página diretamente nesse frame, atualiza a tabela de páginas com o número do frame e define o bit de validade como ativo. Nenhuma substituição é necessária, apenas a contagem de páginas carregadas é atualizada.

**Cenário 2: Memória cheia (substituição)**
Se não houver frames livres, o sistema reconhece que a memória está cheia e aciona o algoritmo de substituição selecionado (FIFO ou Clock). O algoritmo determina qual página será removida — no FIFO, a mais antiga; no Clock, a primeira com r_bit = 0. Após identificar a vítima, a página é invalidada, o novo conteúdo é carregado no mesmo frame e a tabela de páginas é atualizada, garantindo a coerência entre memória lógica e física.

---

## 3. Análise Comparativa FIFO vs Clock

### 3.1 Resultados dos Testes

Preencha a tabela abaixo com os resultados de pelo menos 3 testes diferentes:

| Descrição do Teste | Total de Acessos | Page Faults FIFO | Page Faults Clock | Diferença|
|-------------------|------------------|------------------|-------------------|-----------|
| Teste 1 - Básico  |        8         |        5         |         5         |     0     |
| Teste 2 - Memória Pequena | 10       |        10        |         10        |     0     |
| Teste 3 - Simples |        7         |        4         |        4          |    0      |
| Teste Próprio 1   |         9        |         6        |        6          |     0     |

### 3.2 Análise

Com base nos resultados acima, responda:

1. **Qual algoritmo teve melhor desempenho (menos page faults)?**
Tanto quanto o Clock e FIFO tiveram a mesma quantidade de page faults

2. **Por que você acha que isso aconteceu?** Considere:
   O Clock usa o bit de referência (R-bit) para preservar páginas recentemente acessadas, enquanto o FIFO substitui simplesmente a mais antiga. Entretanto, quando o padrão de acessos é sequencial ou não há reutilização significativa de páginas, o R-bit acaba sendo zerado em todas e o Clock se comporta de forma semelhante ao FIFO, o que pode explicar os resultados iguais.

3. **Em que situações Clock é melhor que FIFO?**
   O Clock tende a ter melhor desempenho quando páginas acessadas recentemente tendem a ser reutilizadas,por exemplo, em laços de repetição curtos ou padrões onde um pequeno conjunto de páginas é frequentemente referenciado. Nesses casos, o R-bit impede que páginas “quentes” sejam substituídas, reduzindo o número de page faults.

4. **Houve casos onde FIFO e Clock tiveram o mesmo resultado?**
   - Por que isso aconteceu?
   Sim, especialmente quando todas as páginas têm uso semelhante ou o padrão de acesso é sequencial. Nessas situações, o R-bit do Clock não consegue diferenciar páginas recentes de antigas, fazendo com que o algoritmo se comporte de forma muito parecida com FIFO.

5. **Qual algoritmo você escolheria para um sistema real e por quê?**
Para um sistema real, o Clock é preferível, pois ele combina simplicidade com eficiência, preserva páginas recentemente usadas e reduz page faults em padrões típicos de programas, ao mesmo tempo que exige menos manutenção de listas do que um LRU completo.

---

## 4. Desafios e Aprendizados

### 4.1 Maior Desafio Técnico

Descreva o maior desafio técnico que seu grupo enfrentou durante a implementação:



### 4.2 Principal Aprendizado

Descreva o principal aprendizado sobre gerenciamento de memória que vocês tiveram com este projeto:



---

## 5. Vídeo de Demonstração

**Link do vídeo:** [Insira aqui o link para YouTube, Google Drive, etc.]

### Conteúdo do vídeo:

Confirme que o vídeo contém:

- [ ] Demonstração da compilação do projeto
- [ ] Execução do simulador com algoritmo FIFO
- [ ] Execução do simulador com algoritmo Clock
- [ ] Explicação da saída produzida
- [ ] Comparação dos resultados FIFO vs Clock
- [ ] Breve explicação de uma decisão de design importante

---

## Checklist de Entrega

Antes de submeter, verifique:

- [X] Código compila sem erros conforme instruções da seção 1.1
- [X] Simulador funciona corretamente com FIFO
- [X] Simulador funciona corretamente com Clock
- [X] Formato de saída segue EXATAMENTE a especificação do ENUNCIADO.md
- [X] Testamos com os casos fornecidos em tests/
- [ ] Todas as seções deste relatório foram preenchidas
- [X] Análise comparativa foi realizada com dados reais
- [ ] Vídeo de demonstração foi gravado e link está funcionando
- [ ] Todos os integrantes participaram e concordam com a submissão

---
## Referências
https://www.youtube.com/watch?v=XGxFhz8ZbtA


---
