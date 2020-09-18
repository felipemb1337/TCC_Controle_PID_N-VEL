#include <TimerOne.h>
#include <TimerThree.h>
#include <TimerFour.h>
#include <TimerFive.h>

//As linhas de códigos acima estão chamando os recursos de bibliotecas referente a manipulação de Timers no arduino

int gatilho = 9,echo = 8, controlpin = 2,SINAL_PWM,caracter;                                                                                                                //Criação de variáveis do tipo inteiro de 16 bits e atribuição de nomes para pinos físicos de entrada e saída
unsigned long currentTime, loopTime;                                                                                                                                        //Criação de variáveis de 32 bits positivas
float SINAL_SAIDA, ERRO,ERRO_ACUMULADO,ERRO_ANTERIOR,TAXA_ERRO,SETPOINT,LARGURA_BANDA,kp=2.5,ki=0.02,kd=5.0,tempo,distancia_cm,P,I,D,dt,t,tempoatual,limite_I,limite_D;    //Criação de Variáveis do tipo Float de 32 bits

void setup() 
{

  Serial.begin(250000);                                   //Inicia e configura a comunicação serial RS232 em 250000 bits por segundo 
  pinMode(gatilho,OUTPUT);                                //Define o pino para emissão do sinal para o sensor ultrassônico como pino de saída
  pinMode(echo,INPUT);                                    //Define o pino para recebimento do sinal do ultrassônico como entrada
  digitalWrite(gatilho,LOW);                              //Inicia o pino de acionamento do ultrassônico como nível lógico 0
  delayMicroseconds(10);                                  //Espera 10 microssegundos antes de ir para próxima instrução

  pinMode(controlpin,OUTPUT);                             //Define o pino de controle do sistema como saída                             
  digitalWrite(controlpin,LOW);                           //Inicia o pino de controle como nível lógico baixo  

  Timer1.initialize(1000000);                             //Inicia e configura a Interrupção de Timer 1 para ocorrer a cada 1 segundo ou 1000000 micro segundos
  Timer1.attachInterrupt(Nivel);                          //Associa a interrução de Timer 1 a subrotina "Nivel", sempre que ocorrer uma interrupção de timer 1 a rotina Nivel será executada
  Timer3.initialize(200);                                 //Inicia e configura a Interrupção de Timer 3 para ocorrer a cada 0.2 milisegundos ou 200 micro segundos, esta interrupção será responsável por modular o sinal de controle PWM                                 
  Timer5.initialize(75000);                               //Inicia e configura a Interrupção de Timer 5 para ocorrer a cada 75 milisegundos ou 75000 micro segundos
  Timer5.attachInterrupt(Acao_ID);                        //Associa a interrução de Timer 5 a subrotina "Acao_ID", sempre que ocorrer uma interrupção de timer 5 a rotina "Acao_ID" será executada                       
  Timer4.initialize(65000);                               //Inicia e configura a Interrupção de Timer 4 para ocorrer a cada 65 milisegundos ou 65000 micro segundos                                 
  Timer4.attachInterrupt(Leitura_Ultrassonico);           //Associa a interrução de Timer 4 a subrotina "Leitura_Ultrassonico", sempre que ocorrer uma interrupção de timer 4 a rotina "Leitura_Ultrassonico" será executada

  SETPOINT=12.0;                                          //Define o Setpoint inicial para 15Cm (Nível desajado do tanque)
}

void controle()                                           //Subrotina para execução da ação de controle proporcional
{
  ERRO=SETPOINT-distancia_cm;                             //Calculo do erro em cm

  ERRO=ERRO*(1023.0/LARGURA_BANDA);                       //Conversão do erro de cm para um valor digital de 0 a 1023 para processamento do valor digital

  P=kp*ERRO;                                              //Calculo da ação proporcional

  if(P>1023.0)                                            //Limitador da ação de controle proporcional, esta não pode passar o valor máximo permitido para saída do controlador
  {
    P=1023.0;
  }
  else if(P<-1023.0)                                     //Limitador da ação de controle proporcional em seu valor mínimo                                      
  {
    P=-1023.0;
  }
}

void Acao_ID()                                          //Subrotina para execução das ações de controle integral e derivativa
{
  tempoatual=millis();                                  //Carrega na variável tempoatual o tempo decorrido desde o inicio do programa
  tempoatual = tempoatual/1000;                         //Converte este valor de milisegundos para segundos
  dt=tempoatual-t;                                      //Calcula a diferença de tempo gasta entre a interação desta rotina atual com a interação anterior
  t=millis();                                           //Carrega na variável t o valor decorrido desde o inicio do programa que ao ser rodado novamente esta rotina, será o valor da interação anterior
  t=t/1000;                                             //Converte este valor de milisegundos para segundos                                             

  ERRO=SETPOINT-distancia_cm;                           //Atualiza o valor de erro em centímetros

  ERRO=ERRO*(1023/LARGURA_BANDA);                       //Atualiza o valor de erro em valor digital                       

  if((ERRO<=1023.0)&&(ERRO>=-1023.0))                   //Se o erro digital estiver entre 1023.0 e -1023.0 ou seja dentro da largura de banda  
  {
  ERRO_ACUMULADO=ERRO_ACUMULADO+(dt*ERRO);              //Calculo do erro acumulado em função do intercalo de tempo entre amostragens
  I=ki*ERRO_ACUMULADO;                                  //Calculo da ação de controle Integral

  if(ERRO_ACUMULADO>limite_I)                           //Rotina Auto-Wind-up, o valor de erro acumulado não pode ultrapassar o valor de Máximo de limite_I para que não exceda o espaço de memória ocupado pela variável            
  {
    ERRO_ACUMULADO=limite_I;
  }
  if(ERRO_ACUMULADO<-limite_I)                           //Rotina Auto-Wind-up, o valor de erro acumulado não pode ultrapassar o valor de Mínimo de limite_I para que não exceda o espaço de memória ocupado pela variável  
  {
    ERRO_ACUMULADO=-limite_I;
  }

  if(I>1023.0)                                            //Limitador da ação de controle integral, esta não pode passar o valor máximo permitido para saída do controlador
  {
    I=1023.0;
  }
  else if(I<-1023.0)                                      //Limitador da ação de controle proporcional em seu valor mínimo                                       
  {
    I=-1023.0;
  }


  TAXA_ERRO=(ERRO-ERRO_ANTERIOR)/dt;                      //Calculo da taxa de erro em fução do tempo de amostragem                        

  D=(kd*TAXA_ERRO);                                       //Calculo da ação de controle derivativo
  
  /* if((TAXA_ERRO>limite_D)||(TAXA_ERRO<-limite_D))      //Fitragem do valor para pequenas oscilações de valores que acionariam o controle derivativo, porém ao decorrer dos teste foi verificado que esta rotina não era necessário 
  {
    D=(kd*TAXA_ERRO);
  }
  else if((TAXA_ERRO<limite_D)&&(TAXA_ERRO>-limite_D))
  {
    TAXA_ERRO=0.0; 
    D=0.0;
  }*/
  if(D>1023.0)                                            //Limitador da ação de controle derivativo, esta não pode passar o valor máximo permitido para saída do controlador    
  {
    D=1023.0;
  }
  else if(D<-1023.0)                                      //Limitador da ação de controle derivativo em seu valor mínimo                                      
  {
    D=-1023.0;
  }
}  

else if((ERRO>1023.0)||(ERRO<-1023.0))                    //Se o erro atingir um valor que esteja acima da largura de banda as ações de controle são zeradas 
{
  D=0.0;
  I=0.0; 
  ERRO_ACUMULADO=0.0; 
}
  
  ERRO_ANTERIOR=ERRO;                                     //Atualização do erro para utilização da próxima iteração desta rotina como erro anterior   
}

void Leitura_Ultrassonico()                               //Subrotina para emissão de sinal e calculo do tempo gasto entre emissão e recebimento do sinal
{
  digitalWrite(gatilho, HIGH);                           
  delayMicroseconds(10);
  digitalWrite(gatilho, LOW);
  tempo = pulseIn(echo, HIGH);
}

void Nivel()                                              // Subrotina para envio do nível calculado pelo arduino e leitura dos valores enviados serialmente pelo Labview de kp,ki e kd a cada 1 segundo                                       
{
  
Serial.println(distancia_cm);                             //Envia pela serial o nivel do tanque calculado pelo arduino através da elitura do ultrassônico
 
    if(Serial.available())                                //Se for identificado dados no Buffer de recepção serial
    {
      caracter=Serial.read();                             //Amazena dentro da variável caracter o valor lido pela serial

      switch (caracter)                                   //Analiza a variável caracter 
      {
      case 'P':                                           //Caso o primeiro byte seja 'P'
      caracter=Serial.parseInt();                         //Converte o próximo byte recebido em variável do tipo inteiro
      kp=caracter/20.0;                                   //O ganho Kp será o valor inteiro recebido pela serial dividido por um fator de ajuste a conversão em um valor real
      break;                                              //Fim desta rotina Case
      
      case 'I':                                           //Caso o primeiro byte seja 'I'
      caracter=Serial.parseInt();                         //Converte o próximo byte recebido em variável do tipo inteiro
      ki=caracter/500.0;                                  //O ganho Ki será o valor inteiro recebido pela serial dividido por um fator de ajuste a conversão em um valor real
      break;                                              //Fim desta rotina Case
      
      case 'D':                                           //Caso o primeiro byte seja 'D'
      caracter=Serial.parseInt();                         //Converte o próximo byte recebido em variável do tipo inteiro
      kd=caracter/20.0;                                   //O ganho Kd será o valor inteiro recebido pela serial dividido por um fator de ajuste a conversão em um valor real                                   
      break;                                              //Fim desta rotina Case
      
      case 'S':                                           //Caso o primeiro byte seja 'S'
      caracter=Serial.parseInt();                         //Converte o próximo byte recebido em variável do tipo inteiro
      SETPOINT=caracter/15.937;                           //O valor de SETPOINT será o valor inteiro recebido pela serial dividido por um fator de ajuste a conversão em um valor real
      break;                                              //Fim desta rotina Case
      default:                                            //Caso não faça nenhuma instrução das rotinas case 
      break;                                              //Fim da rotina default
      }
    }
}


void loop() 
{
  LARGURA_BANDA=0.3*SETPOINT;                            //Definição da faixa de controle 
  limite_I=(1023.0/ki);                                  //Definição do limite máximo aceitavel para o erro acumulado 
  //limite_D=(0.01/LARGURA_BANDA)*13640.0;               //Definição do limite máximo aceitavel para a sensibilidade da ação derivativa, porém como já mencionado acima esta ação não foi necessária                       
    if(limite_I<1023.0)                                  //Garantia que o limite máximo aceitavel para o erro acumulado não ultrapasse o valor de 1023                                   
    {
      limite_I=1023.0;
    }
  distancia_cm = tempo / 29.4/ 2;                        //distancia_cm calculada com base no tempo de leitura entre um pulso emitido pelo ultrassônico e o correspondente pulso recebido 
  distancia_cm = 36.2-distancia_cm;                      //Nível calculado  
  controle();                                            //Executa a subrotina controle() 

    SINAL_SAIDA=P+I+D;                                   //Soma das ações de controle Proporcional, Integral e Derivativa

    if(SINAL_SAIDA>1023.0)                               //Se a variavel "SINAL_SAIDA" que é a soma das três ações P,I e D, for maior que 1023 
    {
      SINAL_SAIDA=1023.0;                                //Limita a saída máxima em 1023 
    }
    else if(SINAL_SAIDA<0.0)                             //Se a variavel "SINAL_SAIDA" que é a soma das três ações P,I e D, for menor que 0 (PWM não pode assumir valor negativo)  
    {
      SINAL_SAIDA=0.0;                                   //Limita a saída máxima em o (Bomba desligada)                                   
    }

    SINAL_PWM=SINAL_SAIDA;                               //O Sinal de controle será o valor inteiro da variável SINAL_SAIDA que é a soma das três ações P,I e D  
    Timer3.pwm(controlpin,SINAL_PWM );                   //Execulta ação de controle através do sinal PWM calculado
}
