Descripcion y diseño logico del firmware

Tomaremos al circuito como una maquina de estados que va alternando entre los tres modos de funcionamiento:

- Performance
- Configuration
- OFF

En performance, el circuito hace lecturas periodicas de los sensores y los publica usando MQTT en los diferentes topicos definidos en la jerarquia topica.

En configuration, el circuito deja de hacer lecturas de los sensores y el usuario podra configurar diferentes parametros mediante
el potenciometro y el valor que se configura se devolvera en la pantalla LCD para saber en que valor se esta configuracion y 
que valor se ha configurado en el modo performance. 

En el modo OFF, no apaga el circuito y no realiza ninguna accion.

La alternancia entre los modos se controla con dos botones. Un boton cambiara entre performance y configuration que servira tambien
una señal para guardar el dato configurado. Habra otro boton para apagar el circuito. En caso de querer volver a encenderlo se hace
desde el otro boton que intercambia entre performance o configuration


Para saber en que modo esta el circuito tenemos 3 leds que indican:
- Rojo --> OFF
- Verde --> Performance
- Amarillo --> Configuration

Componentes 

Input
- Fotoresistencia --> ADC
- Humedad/Temperatura --> ADC
- Potenciometro --> ADC
- 2 botones --> Entrada digital

Output
- Pantalla LCD --> bus I2C
- LEDs --> Salida digital
- Publicacion --> MQTT

