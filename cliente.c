/******************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos
 **
 ** Aluno: Nº:98763 Nome: Kevin Rodrigues Borges
 ** Nome do Módulo: cliente.c v3
 ** Descrição/Explicação do Módulo: A explicação do módulo é feita ao longo do código.
 **
 **
 ******************************************************************************/
#include "common.h"
#include "utils.h"
#include <sys/stat.h>

/* Variáveis globais */
Passagem pedido;                        // Variável que tem o pedido enviado do Cliente para o Servidor
int pidServidor;                        // Variável que tem o PID do processo Servidor
char passagemIniciada = FALSE;          // Variável que indica que o Servidor já enviou sinal de início da passagem

/* Protótipos de funções */
int getPidServidor();                   // C1: 
int armaSinais();                       // C2: 
Passagem getDadosPedidoUtilizador();    // C3: 
int escrevePedido(Passagem);            // C4: 
int configuraTemporizador();            // C5: 
void trataSinalSIGUSR1(int);            // C6: 
void trataSinalSIGTERM(int);            // C7: 
void trataSinalSIGHUP(int);             // C8: 
void trataSinalSIGINT(int);             // C9: 
void trataSinalSIGALRM(int);            // C10: 

/**
 * Main: Processamento do processo Cliente
 * 
 * @return int Exit value
 */
int main() {    // Os alunos em princípio não deverão alterar esta função
    // C1
    pidServidor = getPidServidor();
    exit_on_error(pidServidor, FILE_SERVIDOR);
    // C2
    exit_on_error(armaSinais(), "armaSinais");
    // C3
    pedido = getDadosPedidoUtilizador();
    exit_on_error(pedido.tipo_passagem, "getDadosPedidoUtilizador");
    // C4
    exit_on_error(escrevePedido(pedido), "escrevePedido");
    // C5
    exit_on_error(configuraTemporizador(), "configuraTemporizador");
    // Aguarda processamento por parte do Servidor
    while (TRUE) {
        debug("", "Aguarda processamento por parte do Servidor");
        pause();
    }
}

/**
 *  O módulo Cliente é responsável pelo pedido das passagens. Este módulo será utilizado para solicitar a passagem das viaturas pelas portagens.
 *  Após identificação da viatura, é realizado o pedido da respetiva passagem, ficando este módulo a aguardar até que o processamento esteja concluído.
 *  Assim, definem-se as seguintes tarefas a desenvolver:
 */

/**
 * C1   Lê a informação acerca do PID do Servidor de Passagens que deve estar registado no ficheiro FILE_SERVIDOR. 
 *      No caso de o ficheiro não existir ou de não ter um PID registado no ficheiro, dá error C1 "<Problema>" e termina o processo Cliente.
 *      Caso contrário, assume que a única informação no ficheiro (em formato de texto) é o PID do Servidor
 *      (pai, único PID do lado do Servidor que este Cliente conhece), e dá success C1 "<PID Servidor>";
 *
 * @return int Sucesso
 */
int getPidServidor() {
    debug("C1", "<");
    //Abre o ficheiro FILE_SERVIDOR
    FILE *fp = fopen(FILE_SERVIDOR, "r");
    
    //Se o pointer fp ( obtido através do fopen() ) for NULL, 
    //é mostrado um erro e retornado -1 para que no main o exit_on_error saia do programa
    if (fp == NULL) { 
        error( "C1", "Erro na abertura do ficheiro servidor.pid");
        return -1;
    }

    //Criação de uma string
    char Pid_servidor[10];
    //Usando a funcao my_fgets é copiado o Pid do servidor localizado no ficheiro FILE_SERVIDOR
    //para a variavel Pid_servidor, caso a operacao de leitura do ficheiro falhe é retornado -1
    if (my_fgets(Pid_servidor, 10, fp) == NULL) {
        error("C1", "Erro na leitura do ficheiro");
        return -1;
    }
    //Como o my_fgets retorna uma string é transformada a string Pid_servidor num inteiro usando a funcao atoi() (ascii to integer)
    int PID = atoi(Pid_servidor);
    //Verificação se o pid lido é valido (isto é, se o mesmo é menor ou igual a 0)
    if (PID <= 0) {
        error("C1", "PID inválido");
    }
    //É fechado o ficheiro FILE_SERVIDOR
    fclose(fp);
    //É retornada uma mensagem de sucesso
    success("C1", "<%d>", PID);

     debug("C1", ">");
    return PID;
}

/**
 * C2   Pede ao Condutor (utilizador) que preencha os dados referentes à passagem da viatura (Matrícula e Lanço),
 *      criando um elemento do tipo Passagem com essas informações, e preenchendo o valor pid_cliente com o PID do seu próprio processo Cliente.
 *      Em caso de ocorrer qualquer erro, dá error C2 "<Problema>", e termina o processo Cliente;
 *      caso contrário dá success C2 "Passagem do tipo <Normal | Via Verde> solicitado pela viatura com matrícula <matricula> para o Lanço <lanco> e com PID <pid_cliente>";
 *
 * @return Passagem Elemento com os dados preenchidos. Se tipo_passagem = -1, significa que o elemento é imválido
 */
Passagem getDadosPedidoUtilizador() {
    debug("C2", "<");

    //Criação de uma variavel do tipo String
    char temp[20];
    //Criacao de uma variavel do tipo Passagem
    Passagem p;
    p.tipo_passagem = -1;   // Por omissão, retorna valor inválido
    // Pedido do tipo de passagem ao cliente
    printf("Introduza o tipo de passagem: "); 
    //Usanfo a funcao my_gets é copiado o texto do standart input para a variavel temp
    my_gets(temp, 20);
    //Ao tipo de passagem de p é acossiado o numero inteiro lido ("traduzido" para inteiro usando atoi()) 
    p.tipo_passagem = atoi(temp);
    //Verificação do tipo de passagem, e se o mesmo corresponder a um dos valores esperado 
    //é mudado o conteudo da string temp para o texto correspondete a esse tipo de passagem usando strcpy
    //No caso de ser inválido é alterado para -1, sinalizando assim um pedido inválido
    if (p.tipo_passagem == 1){
        strcpy(temp, "Normal");
    }else if (p.tipo_passagem == 2){
        strcpy(temp, "Via Verde");
    }else{
        strcpy(temp, "Invalido");
        error("C2", "Tipo de passagem inválido");
        p.tipo_passagem = -1;
    }
    // Pedido da matricula ao cliente
    printf("Introduza a Matrícula: ");
    //Usando a funcao my_gets é copiado o texto do standart input para p.matricula
    my_gets(p.matricula, 9);
    //Se este texto previamente lido do standart input for NULL ou com um tamanho nulo (0), é retornado um erro, e o programa é fechado
    if (p.matricula == NULL || strlen(p.matricula) == 0) {
        error("C2", "Matricula inválida");
        exit(1);
    }
    //Pedido do lanco ao cliente
    printf("Introduza o Lanço: ");
    //Usando a funcao my_gets é copiado o texto do standart input para p.lanco
    my_gets(p.lanco, 50);
    //Se este texto previamente lido do standart input for NULL ou com um tamanho nulo (0), é retornado um erro, e o programa é fechado
    if (p.lanco == NULL || strlen(p.lanco) == 0) {
        error("C2", "Lanço inválido");     
        exit(1);
    }
    //é acossiado o pid do processo atual (usando getpid()) ao p.pid_cliente
    p.pid_cliente = getpid();
    //é retornada uma mensagem de sucesso
    success("C2", "Passagem do tipo %s solicitado pela viatura com matrícula %s para o Lanço %s e com PID %d", temp, p.matricula, p.lanco, p.pid_cliente);

    debug("C2", ">");

    return p;
}

/**
 * C3   Arma os sinais SIGUSR1 (ver C6), SIGTERM (ver C7), SIGHUP (ver C8), SIGINT (ver C9), e SIGALRM (ver C10),
 *      dando, no fim de os armar, a mensagem success C3 "Armei sinais";
 *
 * @return int Sucesso
 */
int armaSinais() {
    debug("C3", "<");
    //Os 5 sinais são armados usando a mesma funcao (signal), e a cada um deles é correspondido o "handler" daquele sinal
    signal(SIGUSR1,trataSinalSIGUSR1);
    signal(SIGTERM, trataSinalSIGTERM);
    signal(SIGHUP, trataSinalSIGHUP);
    signal(SIGINT,trataSinalSIGINT);
    signal(SIGALRM,trataSinalSIGALRM);
    //É apresentada uma mensagem de sucesso
    success("C3", "Armei sinais");

    debug("C3", ">");
    return 0;
}

/**
 * C4   Valida se o ficheiro com organização FIFO (named pipe) FILE_PEDIDOS existe.
 *      Se esse ficheiro FIFO não existir, dá error C4 e termina o processo Cliente.
 *      Caso contrário, escreve as informações do elemento Passagem (em formato binário) nesse FIFO FILE_PEDIDOS.
 *      Em caso de erro na escrita, dá error C4 e termina o processo Cliente, caso contrário, dá success C4 "Escrevi FIFO";
 *
 * @return int Sucesso
 */
int escrevePedido(Passagem dados) {
    debug("C4", "<");
    //É criada uma estrutura (a mesma servirá para testar se o fifo existe e se o mesmo é de facto um fifo)
    struct stat st;
    // Valida se o ficheiro FIFO existe. 
    int result = stat("nomeFifo", &st); 
    if (!result){
        error("C4", "O ficheiro FIFO não existe.");
    }
    // Valida se o ficheiro é de facto um FIFO. 
    if (!S_ISFIFO(st.st_mode)) {
        error("C4", "O ficheiro existe mas não é um FIFO!"); 
        return -1;
    }
    //É aberto o ficheiro FIFO FILE_PEDIDOS usando a funcao fopen()
    FILE *fp = fopen(FILE_PEDIDOS, "wb"); //Verificar se queremos sobrescrever ou apenas escrever
    //Se o pointeiro retornado pela funcao fopen() for NULL é apresentada uma mensagem de erro e é retornado -1
    if (fp == NULL) {
        error("C4", "<O Ficheiro não existe>");
        return -1;
    }
    //Usando a funcao fwrite(), é escrito no FIFO,uma estrutura (Uma Passagem)
    fwrite(&dados, sizeof(Passagem), 1, fp);
    //É fechado o ficheiro FIFO FILE_PEDIDOS
    fclose(fp);
    //É retornada uma mensagem de sucesso
    success("C4", "Escrevi FIFO");

    debug("C4", ">");
    return 0;
}

/**
 * C5   Configura um alarme com o valor de MAX_ESPERA segundos (ver C10),
 *      dá success C5 "Inicia Espera de <MAX_ESPERA> segundos",
 *      e fica a aguardar o resultado do processamento do pedido por parte do Servidor.
 *
 * @return int Sucesso
 */
int configuraTemporizador() {
    debug("C5", "<");
    //usando a funcao alarm(), o sinal SIGALARM será enviado a este mesmo processo se se passarem MAX_ESPERA segundos
    alarm(MAX_ESPERA);
    //É apresentada uma mensagem de sucesso
    success("C5", "Inicia espera de %d segundos", MAX_ESPERA);

    debug("C5", ">");
    return 0;
}

/**
 * C6   O sinal armado SIGUSR1 serve para o Servidor Dedicado indicar que o processamento da passagem foi iniciado.
 *      Se o Cliente receber esse sinal, dá success C6 "Passagem Iniciada", assinala que o processamento iniciou,
 *      e retorna para continuar a aguardar a conclusão do processamento do lado do Servidor Dedicado;
 */
void trataSinalSIGUSR1(int sinalRecebido) {
    debug("C6", "<");
    //O valor da variavel global é alterado para TRUE
    passagemIniciada = TRUE;
    //É apresentada uma mensagem de sucesso
    success("C6", "Passagem Iniciada");

    debug("C6", ">");
}

/**
 * C7   O sinal armado SIGTERM serve para o Servidor Dedicado indicar que o processamento da passagem foi concluído.
 *      Se o Cliente receber esse sinal, dá success C7 "Passagem Concluída", e termina o processo Cliente.
 *      ATENÇÂO: Deverá previamente validar que anteriormente este Cliente já tinha recebido o sinal SIGUSR1 (ver C6),
 *               indicando que o processamento do lado do Servidor Dedicado teve início,
 *               caso contrário, em vez de sucesso, dá error C7 e termina o processo Cliente;
 */
void trataSinalSIGTERM(int sinalRecebido) {
    debug("C7", "<");
    //Se a variavel global passagemIniciada for igual a TRUE, uma mensagem de sucesso é apresentada e o processo é fechado (retornando 0)
    //Caso a variael global for igual a FALSE, uma mensagem de erro é apresentada e o processo é fechado (retornando 1)
    if (passagemIniciada == TRUE) {

        success("C7", "Passagem Concluída");
        exit(0);

    }else {

        error("C7", "<Problema>");
        exit(1);
    }


    debug("C7", ">");
}

/**
 * C8   O sinal armado SIGHUP serve para o Servidor Dedicado indicar que o processamento a passagem não foi concluído.
 *      Se o Cliente receber esse sinal, dá success C8 "Processo Não Concluído e Incompleto", e termina o processo Cliente;
 */
void trataSinalSIGHUP(int sinalRecebido) {
    debug("C8", "<");
    //Uma mensagem de sucesso é apresentada e o programa é fechado (retornado 0)
    success("C8", "Processo Não Concluído e Incompleto");
    exit(0);

    debug("C8", ">");
}

/**
 * C9   O sinal armado SIGINT serve para que, no caso de o veículo ter uma avaria, ou por outro motivo qualquer,
 *      o condutor (utilizador) possa cancelar o pedido do lado do Cliente, usando o atalho <CTRL+C>.
 *      Se receber esse sinal (do utilizador via Shell), o Cliente envia o sinal SIGHUP ao Servidor,
 *      para que esta passagem seja sinalizada como anomalia, dá success C9 "Processo Cancelado pelo Cliente", 
 *      e retorna para aguardar que o Servidor Dedicado conclua o processo
 *      (o Servidor Dedicado deverá mais tarde enviar o sinal SIGHUP a este Cliente, ver C8);
 */
void trataSinalSIGINT(int sinalRecebido) {
    debug("C9", "<");
    //É enviado um sinal de Hangup ao servidor (Usando o pid obtido anteriormente pela leitura do ficheiro servidor.pid)
    kill(pidServidor, SIGHUP);
    //É apresentada uma mensagem de sucesso
    success("C9", "Processo Cancelado pelo Cliente");

    debug("C9", ">");
}

/**
 * C10  O sinal armado SIGALRM serve para que, se o Cliente em C5 esperou mais do que MAX_ESPERA segundos sem resposta,
 *      o Cliente envia o sinal SIGHUP ao Servidor, para que esta passagem seja sinalizada como anomalia,
 *      dá success C10 "Timeout Cliente", e retorna para aguardar que o Servidor Dedicado conclua o processo
 *      (o Servidor Dedicado deverá mais tarde enviar o sinal SIGHUP a este Cliente, ver C8).
 */
void trataSinalSIGALRM(int sinalRecebido) {
    debug("C10", "<");
    //É enviado um sinal de Hangup ao servidor (Usando o pid obtido anteriormente pela leitura do ficheiro servidor.pid)    
    kill(pidServidor, SIGHUP);
    //É apresentada uma mensagem de sucesso
    success("C10", "Timeout Cliente");

    debug("C10", ">");
}