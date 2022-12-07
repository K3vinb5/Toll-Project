/******************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos
 **
 ** Aluno: Nº: 98763 Nome: Kevin Rodrigues Borges
 ** Nome do Módulo: cliente.c v1
 ** Descrição/Explicação do Módulo: 
 **
 **
 ******************************************************************************/
#include "common.h"
#include "utils.h"
// #define DEBUG_MODE FALSE             // To disable debug messages, uncomment this line

/* Variáveis globais */
Mensagem mensagem;                      // Variável que tem o pedido enviado do Cliente para o Servidor
int msgId;                              // Variável que tem o ID da Message Queue
char passagemIniciada = FALSE;          // Variável que indica que o Servidor já enviou sinal de início da passagem

/* Protótipos de funções */
int getMsg();                           // C1: 
Passagem getDadosPedidoUtilizador();    // C2: 
int enviaPedido( Passagem, int );       // C3: 
Mensagem recebeMensagem( int );         // C4: 
void pedidoAck();                       // C5: 
void pedidoConcluido( Mensagem );       // C6: 
void pedidoCancelado();                 // C7: 

/**
 * Main: Processamento do processo Cliente
 * 
 * @return int Exit value
 */
int main() {    // Os alunos em princípio não deverão alterar esta função
    // C1
    msgId = getMsg();
    // C2
    Passagem pedido = getDadosPedidoUtilizador();
    // C3
    enviaPedido( pedido, msgId );
    // Aguarda processamento por parte do Servidor
    while (TRUE) {
        debug("Aguarda processamento por parte do Servidor");
        // C4
        mensagem = recebeMensagem( msgId );
        switch (mensagem.conteudo.action) {
            // C5
            case 2: pedidoAck();
                    break;
            // C6
            case 3: pedidoConcluido( mensagem );
                    break;
            // C7
            case 4: pedidoCancelado();
        }
    }
}

/**
 *  O módulo Cliente é responsável pelo pedido das passagens. Este módulo será utilizado para solicitar a passagem das viaturas pelas portagens.
 *  Após identificação da viatura, é realizado o pedido da respetiva passagem, ficando este módulo a aguardar até que o processamento esteja concluído.
 *  Assim, definem-se as seguintes tarefas a desenvolver:
 */

/**
 * C1   Tenta abrir uma fila de mensagens (message queue) IPC que tem a KEY IPC_KEY definida em common.h
 *      (alterar esta KEY para ter o valor do nº do aluno, como indicado nas aulas).
 *      Deve assumir que a fila de mensagens já foi criada.
 *      Em caso de erro, dá error C1 "<Problema>" e termina o Cliente com exit status -1. Caso contrário dá success C1 "<msg id>";
 *
 * @return int a msgID IPC desta Message Queue
 */
int getMsg() {
    debug("C1 <");

    //Ligação á mensagem com key IPC_KEY
    int msgId = msgget(IPC_KEY, 0);
    //Verifica se houve algum erro na aberura:
    if(msgId < 0){
        //ID inválido
        error("C1", "Erro na abertura da fila de mensagens");
        exit(-1);
    }else{
        //ID válido
        success("C1", "%d", msgId);
    }

    debug("C1 >");
    return msgId;
}

/**
 * C2   Pede ao Condutor (utilizador) que preencha os dados referentes à passagem da viatura (Matrícula e Lanço),
 *      criando um elemento do tipo Passagem com essas informações, e preenchendo o valor pid_cliente com o PID do seu próprio processo Cliente.
 *      Em caso de ocorrer qualquer erro, dá error C2 "<Problema>", e termina o processo Cliente;
 *      caso contrário dá success C2 "Passagem do tipo <Normal | Via Verde> solicitado pela viatura com matrícula <matricula> para o Lanço <lanco> e com PID <pid_cliente>";
 *
 * @return Passagem Elemento com os dados preenchidos. Se tipo_passagem = -1, significa que o elemento é inválido
 */
Passagem getDadosPedidoUtilizador() {
    debug("C2 <");
    char temp[20];
    Passagem p;

    p.tipo_passagem = -1;   // Por omissão, retorna valor inválido
    p.pid_cliente = getpid(); //Obtenção do pid do cliente

    //Pedido do tipo de passagem:
    printf("Introduza o tipo de passagem: ");
    my_gets(temp, 20);                                  
    p.tipo_passagem = atoi(temp);
    switch(p.tipo_passagem){
        case 1:
            strcpy(temp, "Normal");
            break;

        case 2:
            strcpy(temp, "Via Verde");
            break;

        default:
            strcpy(temp, "Inválido");
            error("C2", "Tipo de passagem inválido");
            p.tipo_passagem = -1;
            exit(-1);
    }

    //Pedido da matricula ao utilizador:
    printf("Introduza a matrícula: ");
    my_gets(p.matricula, 9);
    if ( strlen(p.matricula)== 0  || p.matricula == NULL){
        error("C2", "Os dados introduzidos são inválidos");
        exit(-1);
    }
    
    //Pedido do lanco ao utilizador:
    printf("Introduza o lanco: ");
    my_gets(p.lanco, 50);
    if ( strlen(p.lanco)== 0  || p.lanco == NULL){
        error("C2", "Os dados introduzidos são inválidos");
        exit(-1);
    }

    success("C2", "Passagem do tipo %s solicitado pela viatura com matrícula %s, para o Lanço %s e com PID %d", temp, p.matricula, p.lanco, p.pid_cliente);
    debug("C2 >");
    return p;
}

/**
 * C3   Envia as informações do elemento Passagem numa mensagem com o tipo de mensagem 1 e action 1 – Pedido para a message queue.
 *      Em caso de erro na escrita, dá error C3 "<Problema>", e termina o processo Cliente com exit code -1.
 *      Caso contrário, dá success C3 "Enviei mensagem";
 *      --> Preenche a variável global mensagem.
 *
 * @param pedido Passagem a ser enviada
 * @param msgId a msgID IPC desta Message Queue
 * @return int Sucesso
 */
int enviaPedido( Passagem pedido, int msgId ) {
    debug("C3 <");
    //Criacao da Estrutura de dados a ser enviada
    Mensagem m;
    int status;

    //Atribuicao de dados aos elementos da mensagem a ser enviada
    m.conteudo.action = 1;
    m.conteudo.dados.pedido_cliente = pedido;
    m.tipoMensagem = 1;

    //Envio da mensagem:
    status = msgsnd( msgId, &m, sizeof(m.conteudo), 0 );
    if (status < 0){
        //Erro no envio
        error("C3", "Erro no envio da Mensagem");
        exit(-1);
    }else{
        //Sucesso no envio
        success("C3", "Enviei mensagem");
    }

    
    debug("C3 >");
    return 0;
}

/**
 * C4   Lê da message queue com msgId uma mensagem cujo tipo de mensagem é igual ao PID deste processo Cliente, 
 *      mensagem essa que poderá vir do Servidor Dedicado. Se houver algum erro dá error C4 "<Problema>"
 *      e termina o Cliente com exit code -1. Caso contrário, dá success C4 "Li mensagem do Servidor".
 *
 * @param msgId a msgID IPC desta Message Queue
 * @return Mensagem a mensagem lida
 */
Mensagem recebeMensagem( int msgId ) {
    debug("C4 <");
    Mensagem mensagem;
    int status;

    status = msgrcv( msgId, &mensagem, sizeof(mensagem.conteudo), getpid(), 0 );
    if (status < 0){
        //Erro na leitura da mensagem
        error("C4", "Erro na Leitura da Mensagem");
        exit(-1);
    }else{
        //Sucesso na leitura da mensagem
        success("C4", "Li mensagem do Servidor");
    }

    debug("C4 >");
    return mensagem;
}

/**
 * C5   Se a mensagem que chegou em C4 veio com action 2 – Pedido ACK,
 *      serve para o Servidor Dedicado indicar que o processamento da passagem foi iniciado.
 *      Se o Cliente receber essa mensagem, dá success C5 "Passagem Iniciada", assinala que o processamento iniciou,
 *      e retorna para aguardar a conclusão do processamento do lado do Servidor Dedicado;
 */
void pedidoAck() {
    debug("C5 <");

    //Mensagem de sucesso, assinalando que o processamento se iniciou
    success("C5", "Passagem Iniciada");
    passagemIniciada = TRUE;
    debug("C5 >");
}

/**
 * C6   Se a mensagem que chegou em C4 veio com action 3 – Pedido Concluído,
 *      serve para o Servidor Dedicado indicar que o processamento da passagem foi concluído.
 *      Se o Cliente receber essa mensagem, que inclui os contadores de estatística atualizados,
 *      dá success C6 "Passagem Concluída com estatísticas: <contador normal> <contador via verde> <contador anomalias>", e termina o processo Cliente. 
 *      ATENÇÂO: Deverá previamente validar que anteriormente este Cliente já tinha recebido a mensagem com action 2 – Pedido ACK (ver C5),
 *               indicando que o processamento do lado do Servidor Dedicado teve início,
 *               caso contrário, em vez de sucesso, dá error C6 e termina o processo Cliente;
 *
 * @param mensagem Mensagem recebida do Servidor Dedicado
 */
void pedidoConcluido( Mensagem mensagem ) {
    debug("C6 <");

    if (passagemIniciada == TRUE){
        
        //Atribuicao dos valores guardados no contador em variaveis (Para melhor compreensão)
        int Normal = mensagem.conteudo.dados.contadores_servidor.contadorNormal;
        int Via_Verde = mensagem.conteudo.dados.contadores_servidor.contadorViaVerde;
        int Anomalias = mensagem.conteudo.dados.contadores_servidor.contadorAnomalias;

        //Mensagem de sucesso
        success("C6", "Passagem Concluída com estatísticas: %d %d %d", Normal, Via_Verde, Anomalias);
        exit(0);

    }else{
        error("C6", "Action = 2 não recebido anteriormente");
        exit(-1);
    }

    debug("C6 >");
}

/**
 * C7   Se a mensagem que chegou em C4 veio com action 4 – Pedido Cancelado,
 *      serve para o Servidor Dedicado indicar que o processamento a passagem não foi concluído.
 *      Se o Cliente receber esse sinal, dá success C7 "Processo Não Concluído e Incompleto", e termina o processo Cliente.
 */
void pedidoCancelado() {
    debug("C7 <");

    //Mensagem de sucesso e terminação do processo cliente
    success("C7", "Processo Não Concluído e Incompleto");
    exit(-1);

    debug("C7 >");
}