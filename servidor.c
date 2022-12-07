/******************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos
 **
 ** Aluno: Nº: 98763 Nome: Kevin Rodrigues Borges
 ** Nome do Módulo: servidor.c v3
 ** Descrição/Explicação do Módulo: A explicação do Módulo é feita ao longo do código.
 **
 **
 ******************************************************************************/
#include "common.h"
#include "utils.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


/* Variáveis globais */
Passagem pedido;                            // Variável que tem o pedido enviado do Cliente para o Servidor
Passagem lista_passagens[NUM_PASSAGENS];    // BD com o Número de pedidos em simultâneo que o servidor suporta
int indice_lista;                           // Índice corrente da Lista, que foi reservado pela função reservaEntradaBD()
Contadores stats;                           // Contadores de estatisticas

/* Protótipos de funções */
int init(Passagem*);                   // S1:   Função a ser implementada pelos alunos
int loadStats(Contadores*);                         // S2:   Função a ser implementada pelos alunos
int criaFicheiroServidor();                         // S3:   Função a ser implementada pelos alunos
int criaFifo();                                     // S4:   Função a ser implementada pelos alunos
int armaSinais();                                   // S5:   Função a ser implementada pelos alunos
Passagem lePedido();                                // S6:   Função a ser implementada pelos alunos
int validaPedido(Passagem);                         // S7:   Função a ser implementada pelos alunos
int reservaEntradaBD(Passagem*, Passagem);          // S8:   Função a ser implementada pelos alunos
int apagaEntradaBD(Passagem*, int);                 //       Função a ser implementada pelos alunos
int criaServidorDedicado(Passagem*, int);           // S9:   Função a ser implementada pelos alunos
void trataSinalSIGINT(int);                         // S10:  Função a ser implementada pelos alunos
void trataSinalSIGHUP(int, siginfo_t*, void*);      // S11:  Função a ser implementada pelos alunos
void trataSinalSIGCHLD(int);                        // S12:  Função a ser implementada pelos alunos
int sd_armaSinais();                                // SD13: Função a ser implementada pelos alunos
int sd_iniciaProcessamento(Passagem);               // SD14: Função a ser implementada pelos alunos
int sd_sleepRandomTime();                           // SD15: Função a ser implementada pelos alunos
int sd_terminaProcessamento(Passagem);              // SD16: Função a ser implementada pelos alunos
void sd_trataSinalSIGTERM(int);                     // SD17: Função a ser implementada pelos alunos


int main() {    // Não é suposto que os alunos alterem nada na função main()
    // S1
    exit_on_error(init(lista_passagens), "Init");
    // S2
    exit_on_error(loadStats(&stats), "loadStats");
    // S3
    exit_on_error(criaFicheiroServidor(), "criaFicheiroServidor");
    // S4
    exit_on_error(criaFifo(), "criaFifo");
    // S5
    exit_on_error(armaSinais(), "armaSinais");

    while (TRUE) {  // O processamento do Servidor é cíclico e iterativo
        // S6
        pedido = lePedido();
        if (pedido.tipo_passagem < 0)
            continue;
        // S7
        if (validaPedido(pedido) < 0)
            continue;
        // S8
        indice_lista = reservaEntradaBD(lista_passagens, pedido);
        if (indice_lista < 0)
            continue;
        // S9
        int pidFilho = criaServidorDedicado(lista_passagens, indice_lista);
        if (pidFilho < 0) {
            apagaEntradaBD(lista_passagens, indice_lista);
            continue;
        } else if (pidFilho > 0) // Processo Servidor - Pai
            continue;

        // Processo Servidor Dedicado - Filho
        // SD13
        exit_on_error(sd_armaSinais(), "sd_armaSinais");
        // SD14
        exit_on_error(sd_iniciaProcessamento(pedido), "sd_iniciaProcessamento");
        // SD15
        exit_on_error(sd_sleepRandomTime(), "sd_sleepRandomTime");
        // SD16
        exit_on_error(sd_terminaProcessamento(pedido), "sd_terminaProcessamento");
    }
}

/**
 *  O módulo Servidor de Passagens é responsável pelo processamento de pedidos de passagem que chegam ao sistema  Scut-IUL.
 *  Este módulo é, normalmente, o primeiro dos dois (Cliente e Servidor) a ser executado, e deverá estar sempre ativo,
 *  à espera de pedidos de passagem. O tempo de processamento destes pedidos varia entre os MIN_PROCESSAMENTO segundos
 *  e os MAX_PROCESSAMENTO segundos. Findo esse tempo, este módulo sinaliza ao condutor de que a sua passagem foi processada.
 *  Este módulo deverá possuir contadores de passagens por tipo, um contador de anomalias e uma lista com capacidade para processar NUM_PASSAGENS passagens.
 *  O módulo Servidor de Passagens é responsável por realizar as seguintes tarefas:
 */

/**
 * S1   Inicia a lista de passagens, preenchendo em todos os elementos o campo tipo_passagem=-1 (“Limpa” a lista de passagens).
 *      Em seguida, dá success S1 "Init Servidor";
 *
 * @return int Sucesso
 */
int init(Passagem* bd) {
    debug("S1", "<");
    //É percorrido o array lista_passagens, colocando o tipo de passagem de todas as passagens a -1
    for (int i = 0; i < NUM_PASSAGENS; i++){
        bd->tipo_passagem = -1; 
        bd++; 
    }
    //É apresentada uma mensagem de sucesso
    success("S1", "Init Servidor");

    debug("S1", ">");
    return 0;
}

/**
 * S2   Deverá manter um contador por cada tipo de passagem (Normal ou Via Verde) e um contador para as passagens com anomalia.
 *      Se o ficheiro FILE_STATS existir na diretoria local, abre-o e lê os seus dados (em formato binário, ver formato em S10.4)
 *      para carregar o valor guardado de todos os contadores. Se houver erro na leitura do ficheiro, dá error S2 "<Problema>",
 *      caso contrário, dá success S2 "Estatísticas Carregadas".
 *      Se o ficheiro FILE_STATS não existir, inicia os três contadores com o valor 0 e dá success S2 "Estatísticas Iniciadas";
 *
 * @return int Sucesso
 */
int loadStats(Contadores* pStats) {
    debug("S2", "<");
    //É aberto o ficheiro FILE_STATS
    FILE *fp; 
    fp= fopen(FILE_STATS, "r");
    //Caso a abertura do mesmo corra mal (fp == NULL), são alterados os "constituintes" da estrutura Contadores (stats), para 0
    if (fp == NULL) {
        
        pStats->contadorNormal = 0;
        pStats->contadorViaVerde = 0;
        pStats->contadorAnomalias = 0;

        success("S2", "Estatísticas Iniciadas");

    }else{
        //Caso a abertura do ficheiro tenha corrido bem é lido o conteudo do mesmo (Caso haja um erro na leitura é retornado uma mensagem de erro)
        if (fread(pStats, sizeof(Contadores), 1, fp) == 0){
            error("S2", "Erro na Leitura do ficheiro estatisticas.dat");
            return -1;
            
        }else{
            //É apresentada uma mensagem de sucesso (significando que o ficheiro foi lido corretamente e que os seus conteudos foram transferidos á variavel pstats)
            success("S2", "Estatísticas Carregadas");
            
        }
        fclose(fp);
    }
    debug("S2", ">");
    return 0;
}

/**
 * S3   Cria o ficheiro FILE_SERVIDOR, e escreve nesse ficheiro o PID do Servidor (em formato de texto).
 *      Se houver erro em qualquer das operações, dá error S3 <Problema>, caso contrário, dá success S3 "<PID Servidor>";
 *
 * @return int Sucesso
 */
int criaFicheiroServidor() {
    debug("S3", "<");
    //É criada uma variavel do tipo String
    char PID[10];
    //Usando a funcao sprintf(), converte-se um inteiro (getpid(), correspondente ao pid do processo atual), para a String PID
    sprintf(PID, "%d", getpid());
    //É aberto o ficheiro FILE_SERVIDOR (servidor.pid)
    FILE *fp = fopen(FILE_SERVIDOR, "w");
    
    if(fp){
        //Caso fp seja valido:

        //Usando a funcao fprintf(), é escrito no ficheiro FILE_SERVIDOR a string PID, 
        //caso isto corra mal é apresentada uma mensagem de erro e é retornado -1
        if (fprintf(fp, PID) == 0){
        error("S3", "Erro na Escrita"); 
        return -1;
        }
        //O ficheiro FILE_SERVIDOR é fechado
        fclose(fp);
        //É apresentada uma mensagem de sucesso
        success("S3", "%s", PID);
    }else{
        //Caso fo não seja válido

        //É apresentada uma mensagem de sucesso
        error("S3", "Erro na abertura do ficheiro");
        return -1;
    }
    debug("S3", ">");
    return 0;
}

/**
 * S4   Cria o ficheiro com organização FIFO (named pipe) FILE_PEDIDOS. 
 *      Se houver erro na operação, dá error S4 "<Problema>", caso contrário, dá  success S4 "Criei FIFO";
 *
 * @return int Sucesso
 */
int criaFifo() {
    debug("S4", "<");
    //Usando a funcao mkfifo(), é criado um ficheiro FIFO FILE_PEDIDOS, caso isto corra mal é apresentada uma mensagem de erro e é retornado -1
    if (mkfifo(FILE_PEDIDOS, 0666) != 0) {
        error("S4", "Não Criei o FIFO");
        return -1;
    }
    //É apresentada uma mensagem de sucesso
    success("S4", "Criei FIFO");

    debug("S4", ">");
    return 0;
}

/**
 * S5   Arma e trata os sinais SIGINT (ver S10), SIGHUP (ver S11) e SIGCHLD (ver S12).
 *      Depois de armar os sinais, dá success S5 "Armei sinais";
 *
 * @return int Sucesso
 */
int armaSinais() {
    debug("S5", "<");
    //É criada uma estrutura, a mesma será usada para armar o sinal SIGHUP usando sigaction()
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = trataSinalSIGHUP;
    //É armado o sinal Hangup, usando sigaction()
    sigaction(SIGHUP, &sa, NULL);
    //Os 3 restantes sinais são armados usando signal(), correspondendo-se a cada um a sua funcao "handler"
    signal(SIGINT, trataSinalSIGINT);
    signal(SIGCHLD, trataSinalSIGCHLD);
    //É retornado uma mensagem de sucesso
    success("S5", "Armei sinais");

    debug("S5", ">");
    return 0;
}

/**
 * S6   Lê a informação do FIFO FILE_PEDIDOS, (em formato binário) que deverá ser um elemento do tipo Passagem.
 *      Se houver erro na operação, dá error S6 "<Problema>", caso contrário, dá success S6 "Li FIFO";
 *
 * @return Passagem Elemento com os dados preenchidos. Se tipo_passagem = -1, significa que o elemento é imválido
 */
Passagem lePedido() {
    debug("S6", "<");
    //É criado uma variavel do tipo Passagem
    Passagem p;
    p.tipo_passagem = -1;   // Por omissão, retorna valor inválido
    //Usando a funcao fopen() é aberto o ficheiro FILE_PEDIDOS, e o seu valor de retorno é acossiado ao pointer fp
    FILE *fp = fopen(FILE_PEDIDOS, "rb");
    
    if (fp) {
    //Caso fp tenha uma valor diferente de 0 (significando que algo de errado aconteceu durante a abertura):

        //O conteúdo do ficheiro FILE_PEDIDOS, é copiado para a variavel p (Uma Passagem)
        if (fread(&p, sizeof(Passagem), 1, fp) < 1){
            //Caso algo dê errado é apresentada uma mensagem de erro
            error("S6", "Erro na leitura do FIFO");
            p.tipo_passagem = -1;
        }else{
            //Caso tudo funcione como devido é apresentada uma mensagem de sucesso
            success("S6", "Li FIFO");
        }
        //O ficheiro FILE_PEDIDOS é fechado
        fclose(fp); 
    }else{
    //Caso fp seja 0, então é apresentada uma mensagem de erro
        if (errno != EINTR){
            error("S6", "Problema na abertura do ficheiro");
        }
    }

    debug("S6", ">");
    return p;
}

/**
 * S7   O Servidor deve validar se o pedido está corretamente formatado. A formatação correta de um pedido será:
 *      •  O Tipo de passagem é válido (1 para pedido Normal, ou 2 para Via Verde);
 *      •  A Matrícula e o Lanço não são strings vazias (não é necessário fazer mais validações sobre o seu conteúdo);
 *      •  O pid_cliente é um valor > 0. Não se fazem validações sobre pid_servidor_dedicado.
 *      Em caso de erro na formatação do pedido, dá error S7, indicando qual foi o erro detetado, 
 *      incrementa o contador de anomalias, ignora o pedido, e recomeça o processo no passo S6. Caso contrário, 
 *      dá success S7 "Chegou novo pedido de passagem do tipo <Normal | Via Verde> solicitado pela viatura com matrícula <matricula> para o Lanço <lanco> e com PID <pid_cliente>";
 *
 * @return int Sucesso
 */
int validaPedido(Passagem pedido) {
    debug("S7", "<");
    //É criada uma variavel do tipo inteiro
    int passagem_valida;
    //Caso o tipo de passagem da variavel dada como argumento seja 1 ou 2 á variavel criada previamente é acossiado o valor 1, caso contrario é acossiado o valor 0
    if (pedido.tipo_passagem == 1 || pedido.tipo_passagem == 2){
        passagem_valida = 1;
    }else{
        passagem_valida = 0;
    }
    //Caso a passagem nao seja valida(Os tipos de passagem validados la em cima nao sao 1 nem 2):
    if (passagem_valida == 0){
        //É apresentada uma mensagem de erro
        error("S7", "Tipo de passagem é invalido");
        //É incremenrado o contadorAnomalias da variavel global stats
        stats.contadorAnomalias++;
        //É enviado um singal Hangup ao cliente (Para que o mesmo se feche)
        kill(pedido.pid_cliente, SIGHUP);
        //Por fim é retornado -1 (Para que se volte ao S6)
        return -1;
    }
    //Caso a string pedido.matricula ou pedido.lanco (pertencentes a estrutura dada como argumento) tenham um tamanho nulo (verificaod com a funcao strlen()):
    if (strlen(pedido.matricula) == 0 || strlen(pedido.lanco) == 0){
        //É apresentada uma mensagem de erro
        error("S7", "Matricula ou lanco vazios");
        //É incremenrado o contadorAnomalias da variavel global stats
        stats.contadorAnomalias++;
        //É enviado um singal Hangup ao cliente (Para que o mesmo se feche)
        kill(pedido.pid_cliente, SIGHUP);
        //Por fim é retornado -1 (Para que se volte ao S6)
        return -1;
    }
    //Caso o inteiro pedido.pid_cliente seja invalido (menor ou igual a 0):
    if (pedido.pid_cliente <= 0){
        //É apresentada uma mensagem de erro
        error("S7", "PID inválido");
        //É incrementado o contadorAnomalias da variavel global stats
        stats.contadorAnomalias++;
        //Por fim é retornado -1 (Para que se volte ao S6)
        return -1;
    }
    
    //Associa-se a string certa a pedido.tipo_passagem, (usando strcpy()) conforme o valor do mesmo (1 ou 2)
    char temp[20];   
    if (pedido.tipo_passagem == 1){
        strcpy(temp, "Normal");
    }else if (pedido.tipo_passagem == 2){
        strcpy(temp, "Via Verde");
    }
    //É apresentada uma mensagem de sucesso
    success("S7", "Chegou novo pedido de passagem do tipo %s solicitado pela viatura com matrícula %s para o Lanço %s e com PID %d", temp, pedido.matricula, pedido.lanco, pedido.pid_cliente);
    
    debug("S7", ">");
    return 0;
}

/**
 * S8   Verifica se existe disponibilidade na Lista de Passagens. Se todas as entradas da Lista de Passagens estiverem ocupadas, 
 *      dá error S8 "Lista de Passagens cheia", incrementa o contador de passagens com anomalia, manda o sinal SIGHUP ao processo 
 *      com PID <pid_cliente>, ignora o pedido, e recomeça o processo no passo S6. 
 *      Caso contrário, preenche uma entrada da lista com os dados deste pedido, incrementa o contador de passagens do tipo de passagem correspondente 
 *      e dá success S8 "Entrada <índice lista> preenchida";
 *
 * @return int Em caso de sucesso, retorna o índice da lista preenchido. Caso contrário retorna -1
 */
int reservaEntradaBD(Passagem* bd, Passagem pedido) {
    debug("S8", "<");
    //É criada uma variavel do tipo inteiro
    int indiceLista = -1;
    //É percorrido o array de Passagens lista_passagens
    for (int i = 0; i < NUM_PASSAGENS; i++) {
        //Caso a posição "atual" do array esteje vazia (o tipo de passagem ser -1):
        if(bd[i].tipo_passagem == -1){
            //Á variavel criada anteriormente é acossiado o valor da posição "atual" do array
            indiceLista = i;
            //É apresentada uma mensagem de sucesso
            success("S8", "Entrada %d preenchida", indiceLista);
            //É acossiado á posição "atual" do array o pedido (dado como argumento)
            bd[indiceLista] = pedido;
            //É Incrementado o stats conforme o tipo de passagem
            if (pedido.tipo_passagem == 1){
                stats.contadorNormal++;
            }else if (pedido.tipo_passagem == 2) {
                stats.contadorViaVerde++;
            }

            debug("S8", ">");
            //É retornado o indiceLista
            return indiceLista;
        }
    }
    //É apresentada uma mensagem de erro
    error("S8", "Lista de Passagens cheia");
    //O contadorAnomalias da variavel global stats é incrementado em 1
    stats.contadorAnomalias++;
    //É enviado um sinal Hangup ao cliente (O pid do cliente está no pedido)
    kill(pedido.pid_cliente, SIGHUP);
    return indiceLista; //retornamos novamente a varaiavel indiceLista, uma vez que a mesma nao foi alterada e por isso é -1

    debug("S8", ">");
}


/**
 * "Apaga" uma entrada da Lista de Passagens, colocando tipo_pedido = -1
 *
 * @return int Sucesso
 */
int apagaEntradaBD(Passagem* bd, int indiceLista) {
    debug("", "<");
    //Adiciona-se indiceLista ao pointer bd para que este aponte para a posição certa no array
    bd+=indiceLista;
    //A essa posição é alterado o tipo de passagem, "apagando" assim a entrada no array de Passagens
    bd->tipo_passagem =-1;

    debug("", ">");
    return 0;
}

/**
 * S9   Cria um processo filho (fork) Servidor Dedicado. Se houver erro, dá error S9 "Fork". 
 *      Caso contrário: O processo Servidor Dedicado (filho) continua no passo SD13, 
 *      e o processo Servidor (pai) completa o preenchimento da entrada atual da Lista de Passagens com o PID do Servidor Dedicado, 
 *      e dá success S9 "Criado Servidor Dedicado com PID <pid Filho>". Em qualquer dos casos, de erro ou de sucesso, recomeça o processo no passo S6;
 *
 * @return int PID do processo filho, se for o processo Servidor (pai), 0 se for o processo Servidor Dedicado (filho), ou -1 em caso de erro.
 */
int criaServidorDedicado(Passagem* bd, int indiceLista) {
    debug("S9", "<");
    //É criado um processo filho usando a funcao fork, e é acossiado uma variavel do tipo inteiro pid_filho ao valor retornado pela funcao fork()
    int pid_filho = fork();
    //Caso a variavel pid_filho tenha o valor de -1 é apresentada uma mensagem de erro e é retornado -1
    if (pid_filho==-1) {
        error("S9", "Fork");
        return -1;
    }
    //Caso a variavel pid_filho seja 0 (Será apenas 0 para o processo filho, e será o pid do processo filho para o processo pai):
    if (pid_filho==0) {
        //Codigo do Filho



    //Caso a variavel pid_filho seja 0 (Será apenas 0 para o processo filho, e será o pid do processo filho para o processo pai):
    }else{
        //Codigo do Pai
        //É acossiado ao array lista_passagens no indice dado por indice_lista o pid do filho (relembrando que como é o pai que executa isto a variavel pid_filho contem o pid do filho)
        lista_passagens[indice_lista].pid_servidor_dedicado = pid_filho;
        //É apresentada uma mensagem de sucesso
        success("S9", "Criado Servidor Dedicado com PID %d", pid_filho); 
    }

    debug("S9", ">");
    //É retornado o pid_filho para que a main possa fazer com que o pai volte a ficar á espera de receber novos sinais e o filhoo vá "trabalhar"
    return pid_filho;
}

/**
 * S10  O sinal armado SIGINT serve para o Diretor da Portagem encerrar o Servidor, usando o atalho <CTRL+C>. 
 *      Se receber esse sinal (do utilizador via Shell), o Servidor dá success S10 "Shutdown Servidor", e depois:
 *      S10.1   Envia o sinal SIGTERM a todos os Servidores Dedicados da Lista de Passagens, 
 *              para que concluam o seu processamento imediatamente. Depois, dá success S10.1 "Shutdown Servidores Dedicados";
 *      S10.2   Remove o ficheiro servidor.pid. Em caso de erro, dá error S10.2, caso contrário, dá success S10.2;
 *      S10.3   Remove o FIFO pedidos.fifo. Em caso de erro, dá error S10.3, caso contrário, dá success S10.3;
 *      S10.4   Cria o ficheiro estatisticas.dat, escrevendo nele o valor de 3 inteiros (em formato binário), correspondentes a
 *              <contador de passagens Normal>  <contador de passagens Via Verde>  <contador Passagens com Anomalia>
 *              Em caso de erro, dá error S10.4, caso contrário, dá success S10.4 "Estatísticas Guardadas";
 *      S10.5   Dá success S10.5 e termina o processo Servidor.
 */
void trataSinalSIGINT(int sinalRecebido) {
    debug("S10", "<");
        //É apresentada uma mensagem de sucesso (Indicando que o sinal foi recebido)
        success("S10", "Shutdown Servidor");
        //S10.1

        //É percorrido o array lista_passagens:
        for ( int i = 0; i < NUM_PASSAGENS ; i++ ) {
            //Se na posição "atual" a ser percorrida o tipo de passagem for diferente de -1 (indicando que existe um processo filho a tratar desse pedido):
            if (lista_passagens[i].tipo_passagem != -1){
                //É enviado um sinal Termination (SIGTERM) ao servidor dedicado que estava a tratar desse mesmo pedido
                kill(lista_passagens[i].pid_servidor_dedicado, SIGTERM);
                
            }

        }
        //É apresentada uma mensagem de sucesso
        success("S10.1", "Shutdown Servidores Dedicados");
        //S10.2

        //Usando a funcao remove(), é eliminado o ficheiro servidor.pid, caso algo corra mal, 
        //é apresentada uma mensagem de erro, caso contrario é apresentada uma mensagem de sucesso
        if (remove("servidor.pid") != 0){
            error("S10.2", "Erro na eliminação do ficheiro servidor.pid");
        }else{
            success("S10.2", "ficheiro servidor.pid eliminado com sucesso");
        }

        //S10.3

        //Usando a funcao remove(), é eliminado o ficheiro pedidos.fifo, caso algo corra mal, 
        //é apresentada uma mensagem de erro, caso contrario é apresentada uma mensagem de sucesso
        if (!remove(FILE_PEDIDOS)){
            success("S10.3", "FIFO removido");
        }else{
            error("S10.3","Erro na remoção do FIFO");
        }


        //S10.4

        //Usando a funcao fopen() é aberto o ficheiro estatisticas.dat (Ou criado caso o mesmo não exista)
        FILE *fp = fopen(FILE_STATS, "w");
        //Caso o pointer (valor dado por fopen()) seja nulo, é retornado uma mensagem de erro
        if (fp == NULL){
            error("S10.4", "Erro na criação do ficheiro estatisticas.dat");
        }
        //Usando a funcao fwrite() é escrito uma estrutura Contadores (no caso a variavel global stats) no ficheiro estatisticas.dat
        fwrite(&stats, sizeof(Contadores), 1, fp);
        //Usando a funcao fclose() é fechado o ficheiro estatisticas.dat
        fclose(fp);
        //É apresentada uma mensagem de sucesso
        success("S10.4", "Estatísticas Guardadas");

        //S10.5

        //É apresentada uma mensagem de sucesso é é fechado o programa (retornando 0)
        success("S10.5", "Servidor Shutdown");
        exit(0); 
    debug("S10", ">");
}

/**
 * S11  O sinal armado SIGHUP serve para o Cliente indicar que deseja cancelar o pedido de processamento a passagem. 
 *      Se o Servidor receber esse sinal, dá success S11 "Cancel", e em seguida, terá de fazer as seguintes ações:
 *      S11.1   Identifica o PID do processo Cliente que enviou o sinal (usando sigaction), dá success S11.1 "Cancelamento enviado pelo Processo <PID Cliente>";
 *      S11.2   Pesquisa na Lista de Passagens pela entrada correspondente ao PID do Cliente que cancelou. Se não encontrar, dá error S11.2.
 *              Caso contrário, descobre o PID do Servidor Dedicado correspondente, dá success S11.2 "Cancelamento <PID Filho>";
 *      S11.3   Envia o sinal SIGTERM ao Servidor Dedicado da Lista de Passagens correspondente ao cancelamento, 
 *              para que conclua o seu processamento imediatamente. Depois, dá success S10.1 "Cancelamento Shutdown Servidor Dedicado", 
 *              e recomeça o processo no passo S6.
 */
void trataSinalSIGHUP(int sinalRecebido, siginfo_t* info , void* sla) {
    debug("S11", "<");
    //É apresentada uma mensagem de sucesso (Indicando que o sinal foi recebido)
    success("S11", "Cancel");

    //S11.1

    //É criada uma variavel do tipo inteiro ao qual é atribuido o pid do sinal que enviou este sinal (neste caso o cliente que decidiu cancelar o pedido)
    int PID_cliente = info->si_pid;
    //É apresentada uma mensagem de sucesso
    success("S11.1", "Cancelamento enviado pelo Processo %d", PID_cliente);

    //S11.2
    //São criadas 2 variaveis do tipo inteiro
    int PID_servidor_dedicado;
    int error = -1;
    //É percorrido o array lista_passagens:
    for (int i = 0; i < NUM_PASSAGENS; i++){
        //Caso o pid do cliente no array da lista de passagens da posicao atualemnte a ser percorrida corresponda com o pid do cliente que mandou o sinal:
        if (lista_passagens[i].pid_cliente == PID_cliente) {
            //Á variavel PID_servidor_dedicado é atribuido o pid do servidor dedicado que está a "tratar" desse pedido
            PID_servidor_dedicado = lista_passagens[i].pid_servidor_dedicado;
            //É libertado o espaco do array colocando o tipo de passagem a -1
            lista_passagens[i].tipo_passagem = -1;
            //É apresentada uma mensagem de sucesso
            success("S11.2", "Cancelamento %d", PID_servidor_dedicado);
            //É incrementa o contadorAnomalias da variavel global stats
            stats.contadorAnomalias++;
            //É atribuido o valor zero á variavel error
            error = 0;
            //Usando break; sai-se do loop imediatamente
            break;
        }
    }
    //Caso a variavel error seja diferente de 0 (O que corresponde a que nao haja nehuma correspondecia entre quem enviou o sinal e os pids dos clientes registados no array):
    if (error){
        //É apresentada uma mensagem de erro
        error("S11.2", "PID do cliente não tem nenhum correspondente na lista de pedidos");
    }

    //S11.3

    //É enviaod um sinal de Terminação (SIGTERM), ao servidor dedicado 
    kill(PID_servidor_dedicado, SIGTERM);
    //É apresentada uma mensagem de erro
    success("S11.3", "Sinal de Cancelamento enviado ao Servidor Dedicado");

    debug("S11", ">");
}

/**
 * S12  O sinal armado SIGCHLD serve para que o Servidor seja alertado quando um dos seus filhos Servidor Dedicado terminar.
 *      Se o Servidor receber esse sinal, dá success S12 "Servidor Dedicado Terminou", e em seguida, terá de fazer as seguintes ações:
 *      S12.1   Identifica o PID do Servidor Dedicado que terminou (usando wait), dá success S12.1 "Terminou Servidor Dedicado <PID Filho>";
 *      S12.2   Pesquisa na Lista de Passagens pela entrada correspondente ao PID do Filho que terminou. 
 *              Se não encontrar, dá error S12.2. Caso contrário, “apaga” a entrada da Lista de Passagens 
 *              correspondente (colocando tipo_passagem=-1), dá success S12.2, e recomeça o processo no passo S6.
 */
void trataSinalSIGCHLD(int sinalRecebido) {
    debug("S12", "<");
    //É apresentada uma mensagem de sucesso (Indicando que o sinal foi recebido)
    success("S12", "Servidor Dedicado Terminou"); //perguntar
    //S12.1

    //Usando a funcao wait(), é obtido o pid da crianca que "morreu"
    int PID_child = wait(NULL); //Still not sure
    //É apresentada uma mensagem de sucesso
    success("S12.1", "Servidor Dedicado Terminou");

    //S12.2
    //É criada uma variavel do tipo inteiro
    int successfully = 0;

    //É percorrido o array lista_passagens:
    for ( int i = 0; i < NUM_PASSAGENS; i++){
        //Caso o pid do servidor dedicado da posicao do array "atual" corresponda com o pid do servidor dedicado (processo filho):
        if ( lista_passagens[i].pid_servidor_dedicado == PID_child){
            //É libertado a posicao "atual" dp array lista_passagens
            lista_passagens[i].tipo_passagem = -1;
            //É apresentada uma mensagem de sucesso
            success("S12.2", "Espaço na lista libertado");
            //É alterado o valor da variavel successfully, para 1
            successfully = 1;
            //Usando break; sai-se do loop imediatamente
            break;
        }
    }
    //Caso a variavel successfully tenha um valor nulo (0)
    if (!successfully) {
        //É apresentada uma mensagem de erro
        error("S12.2", "Espaço na lista não libertado");
    }

    debug("S12", ">");
}

/**
 * SD13 O novo processo Servidor Dedicado (filho) arma os sinais SIGTERM (ver SD17) e SIGINT (programa para ignorar este sinal). 
 *      Depois de armar os sinais, dá success SD13 "Servidor Dedicado Armei sinais";
 *
 * @return int Sucesso
 */
int sd_armaSinais() {
    debug("SD13", "<");
    //São armados 2 sinais usando a funcao signal(), dando como argumentos o sinal em si e o a funcao "handler" desses sinais
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, sd_trataSinalSIGTERM);
    success("SD13", "Servidor Dedicado Armei Sinais");

    debug("SD13", ">");
    return 0;
}

/**
 * SD14 O Servidor Dedicado envia o sinal SIGUSR1, indicando o início do processamento da passagem, ao processo <pid_cliente> 
 *      que pode obter da estrutura Passagem do pedido que “herdou” do Servidor ou da entrada da Lista de Passagens, 
 *      e dá success SD14 "Início Passagem <PID Cliente> <PID Servidor Dedic>";
 *
 * @return int Sucesso
 */
int sd_iniciaProcessamento(Passagem pedido) {
    debug("SD14", "<");
    //É enviado o sinal SIGUSR1 ao cliente (Para que o cliente saiba que o "processamento" do seu pedido já começou)
    kill(pedido.pid_cliente, SIGUSR1);
    //É apresentada uma mensagem de sucesso
    success("SD14", "Início Passagem %d %d ", pedido.pid_cliente, getpid());

    debug("SD14", ">");
    return 0;
}

/**
 * SD15 O Servidor Dedicado calcula um valor de tempo aleatório entre os valores MIN_PROCESSAMENTO e MAX_PROCESSAMENTO, 
 *      dá success SD15 "<Tempo>", e aguarda esse valor em segundos (sleep);
 *
 * @return int Sucesso
 */
int sd_sleepRandomTime() {
    debug("SD15", "<");
    //É atribuida á variavel do tipo inteiro sleep_time um valor qualquer entre MIN_PROCESSAMENTO e MAX_PROCESSAMENTO
    int sleep_time = MIN_PROCESSAMENTO + (my_rand() % MAX_PROCESSAMENTO);
    //Usando a funcao sleep() faz-se com que o procesos "durma" durante sleep_time segundos
    sleep(sleep_time);
    //É apresentada uma mensagem de sucesso
    success("SD15", "Sleeping %d seconds", sleep_time);

    debug("SD15", ">");
    return 0;
}

/**
 * SD16 O Servidor Dedicado envia o sinal SIGTERM, indicando o fim do processamento da passagem, ao processo <pid_cliente>, 
 *      dá success SD16 "Fim Passagem <PID Cliente> <PID Servidor Dedicado>", e termina o Servidor Dedicado;
 *
 * @return int Sucesso
 */
int sd_terminaProcessamento(Passagem pedido) {
    debug("SD16", "<");
    //É enviado um sinal de Terminação (SIGTERM) ao cliente
    kill(pedido.pid_cliente, SIGTERM);
    //É apresentada uma mensagem de sucesso
    success("SD16", "Fim Passagem %d %d ", pedido.pid_cliente, getpid());
    //O porgrama é encerrado (retornando-se 0)
    exit(0);
    debug("SD16", ">");

    return 0;
}

/**
 * SD17 O sinal armado SIGTERM serve para o Servidor indicar que deseja terminar imediatamente o pedido de processamento da passagem.
 *      Se o Servidor Dedicado receber esse sinal, envia o sinal SIGHUP ao <pid_cliente>, 
 *      dá success SD17 "Processamento Cancelado", e termina o Servidor Dedicado.
 */
void sd_trataSinalSIGTERM(int sinalRecebido) {
    debug("SD17", "<");
        //É enviado um sinal hangup ao cliente
        kill(pedido.pid_cliente, SIGHUP);
        //É apresentado uma mensagem de sucesso
        success("SD17", "Processamento Cancelado");
        //O porgrama é encerrado (retornando 0)
        exit(0);

    debug("SD17", ">");
}
