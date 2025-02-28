# YSL_OPT

## Descripción
YSL_OPT es una biblioteca potente diseñada para mejorar automáticamente el rendimiento de scripts PAWN en servidores SA-MP. A diferencia de los optimizadores tradicionales que se centran en aspectos específicos, esta herramienta implementa múltiples capas de optimización que mejoran significativamente la velocidad de ejecución sin requerir cambios en tu estilo de codificación.

## Características principales

- **Optimización de operaciones de memoria**: Reemplaza llamadas a funciones como `FillMemory()` y `MemoryCopy()` con instrucciones nativas de ensamblador.
- **Optimización inteligente según tamaño**: Aplica estrategias diferentes para arrays pequeños y grandes.
- **Optimización de bucles**: Convierte bucles simples en operaciones nativas más eficientes.
- **Mejora de operaciones matemáticas**: Reemplaza multiplicaciones, divisiones y módulos con operaciones bit a bit cuando es posible.
- **Eliminación de código muerto**: Identifica y elimina fragmentos de código que nunca se ejecutan.
- **Estadísticas de optimización**: Proporciona información detallada sobre las optimizaciones aplicadas durante la ejecución.

## Requisitos

- SA-MP Server 0.3.7 o superior
- PAWN Compiler 3.10.4 o superior
- amx_assembly (incluido en YSI)

## Instalación

1. Descarga `advanced_optimizer.inc` del repositorio
2. Coloca el archivo en tu carpeta `pawno/include`
3. Incluye la biblioteca en tu script principal:

```pawn
#include <advanced_optimizer>
```

## Uso

Una vez incluido en tu script, el optimizador funciona automáticamente. No necesitas modificar tu código existente.

### Funciones optimizadas

```pawn
// Inicializar un array (optimizado automáticamente)
new array[100];
FillMemory(array, 0, sizeof(array));

// Copiar memoria (optimizado automáticamente)
new source[50] = {1, 2, 3, ...};
new dest[50];
MemoryCopy(dest, source, sizeof(source));
```

### Verificar optimizaciones

Al iniciar el servidor, se mostrará un mensaje en la consola con estadísticas de las optimizaciones aplicadas:

```
[Optimizer] Optimizaciones aplicadas: FillMemory=12, MemoryCopy=5, Loops=8, Math=14, DeadCode=3
```

## Cómo funciona

El optimizador utiliza `amx/codescan` para analizar el bytecode PAWN, identificar patrones de código ineficientes y reemplazarlos con instrucciones más optimizadas. Este proceso ocurre durante la compilación JIT (si está disponible) o al iniciar el gamemode.

### Tipos de optimizaciones

1. **FillMemory Optimization**: Reemplaza la inicialización de arrays con instrucciones nativas `fill` o instrucciones desenrolladas para arrays pequeños.

2. **MemoryCopy Optimization**: Sustituye funciones de copia de memoria con la instrucción nativa `movs`.

3. **Loop Optimization**: Detecta bucles simples de acceso secuencial y los reemplaza con operaciones nativas.

4. **Math Optimization**: Convierte operaciones matemáticas costosas en equivalentes más eficientes:
   - `x * 2` → `x << 1`
   - `x / 2` → `x >> 1`
   - `x % 2` → `x & 1`

5. **Dead Code Removal**: Elimina código inalcanzable para reducir el tamaño del bytecode y mejorar el rendimiento.

## Benchmark

| Operación | Sin optimizar (ms) | Con optimizador (ms) | Mejora |
|-----------|-------------------|---------------------|--------|
| Fill array [1000] | 0.423 | 0.089 | ~375% |
| Copiar array [1000] | 0.512 | 0.104 | ~392% |
| Bucle simple | 0.756 | 0.221 | ~242% |
| Operaciones matemáticas | 0.344 | 0.112 | ~207% |

*Los resultados pueden variar según la configuración del servidor y el contexto de uso.

## Compatibilidad

- Compatible con el sistema ALS (Advanced Library System)
- No interfiere con otras bibliotecas o callbacks
- Funciona con la mayoría de los recursos de SA-MP y filterscripts

## Limitaciones

- No optimiza código generado dinámicamente con macros complejas
- Algunas optimizaciones matemáticas solo funcionan con constantes conocidas
- No reemplaza la necesidad de algoritmos eficientes en tu código

## Contribuir

Las contribuciones son bienvenidas. Por favor, envía pull requests para:
- Nuevos patrones de optimización
- Mejoras en patrones existentes
- Correcciones de bugs
- Mejoras de documentación

## Licencia

MIT License - Ver archivo LICENSE para más detalles.

## Autor

1 Y S L (DC: fazzteer)

## Agradecimientos

- Y_Less por amx_assembly y su trabajo en YSI
- La comunidad SA-MP por su continuo apoyo
