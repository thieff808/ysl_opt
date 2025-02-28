// YSL_OPT - Optimizador mejorado para PAWN
// Incluye más casos de optimización que el original (Creditos: leHeix)

/* CREDITS: 1 Y S L */

#if defined _YSL_OPT_
    #endinput
#endif
#define _YSL_OPT_

#include <amx/codescan>
#include <amx/opcode>
#include <amx/amx_memory>
#include <amx/amx_header>
#include <amx/asm>
#include <amx/asm_macros>

// Control de optimizaciones
static bool:s_OptimizationsApplied = false;
static optimizations_count[6];  // Contador para cada tipo de optimización

// 1. Mejora del FillMemory original
static stock OptimizeFillMemory(const match[CodeScanner]) {
    new cip = CodeScanGetMatchAddressData(match);
    
    #pragma warning push
    #pragma warning disable 213
    new size = CodeScanGetMatchHole(match, 0),
        value = CodeScanGetMatchHole(match, 1),
        destination = CodeScanGetMatchHole(match, 2),
        bool:is_global = (ReadAmxMemory(cip + 24) == :RelocateOpcode(OPCONST_PRI)) || 
                         (ReadAmxMemory(cip + 16) == :RelocateOpcode(OPPUSH_C));
    #pragma warning pop
    
    CodeScanNOPMatch(match);
    
    new ctx[AsmContext];
    AsmInitPtr(ctx, cip, 32);  // Aumentado el buffer para instrucciones adicionales
    
    // Optimización condicional de tamaño
    if (size <= 8) {
        // Para tamaños pequeños, usar operaciones directas puede ser más rápido
        @emit const.pri value
        if (is_global) {
            @emit const.alt destination
        } else {
            @emit addr.alt destination
        }
        
        // Loop unrolling para pequeños tamaños (más rápido que fill para pocos elementos)
        for (new i = 0; i < size; i++) {
            @emit stor.i         // Almacena y avanza automáticamente
            if (i < size - 1) {  // No incrementar después del último almacenamiento
                @emit add.alt 4  // Avanza al siguiente índice
            }
        }
    } else {
        // Para tamaños grandes, usar fill nativo
        @emit const.pri value
        if (is_global) {
            @emit const.alt destination
        } else {
            @emit addr.alt destination
        }
        @emit fill (size * (cellbits / charbits))
    }
    
    optimizations_count[0]++;
    return 1;
}

// 2. Optimización de copia de memoria
static stock OptimizeMemoryCopy(const match[CodeScanner]) {
    new cip = CodeScanGetMatchAddressData(match);
    
    #pragma warning push
    #pragma warning disable 213
    new size = CodeScanGetMatchHole(match, 0),
        source = CodeScanGetMatchHole(match, 1),
        destination = CodeScanGetMatchHole(match, 2),
        bool:source_is_global = (ReadAmxMemory(cip + 24) == :RelocateOpcode(OPCONST_PRI)),
        bool:dest_is_global = (ReadAmxMemory(cip + 16) == :RelocateOpcode(OPCONST_PRI)) || 
                             (ReadAmxMemory(cip + 16) == :RelocateOpcode(OPPUSH_C));
    #pragma warning pop
    
    CodeScanNOPMatch(match);
    
    new ctx[AsmContext];
    AsmInitPtr(ctx, cip, 40);
    
    if (source_is_global) {
        @emit const.pri source
    } else {
        @emit addr.pri source
    }
    
    if (dest_is_global) {
        @emit const.alt destination
    } else {
        @emit addr.alt destination
    }
    
    // Uso de movs en lugar de implementación manual
    @emit movs (size * (cellbits / charbits))
    
    optimizations_count[1]++;
    return 1;
}

// 3. Optimización de bucles simples
static stock OptimizeSimpleLoop(const match[CodeScanner]) {
    new cip = CodeScanGetMatchAddressData(match);
    new loop_start = CodeScanGetMatchHole(match, 0),
        loop_end = CodeScanGetMatchHole(match, 1),
        iterator = CodeScanGetMatchHole(match, 2),
        array_addr = CodeScanGetMatchHole(match, 3);
    
    // Solo optimizar bucles específicos que acceden secuencialmente
    if (loop_end - loop_start > 100 || loop_end - loop_start <= 0) {
        return 0;
    }
    
    CodeScanNOPMatch(match);
    
    new ctx[AsmContext];
    AsmInitPtr(ctx, cip, 64);
    
    // Usar strenc/movs cuando sea un bucle simple de copia/asignación
    @emit addr.pri array_addr
    @emit const.alt loop_start
    @emit add.pri iterator
    @emit load.s.pri iterator
    
    // Generar instrucciones de copia optimizadas
    @emit movs ((loop_end - loop_start + 1) * 4)
    
    optimizations_count[2]++;
    return 1;
}

// 4. Optimización de operaciones matemáticas redundantes
static stock OptimizeMathOperations(const match[CodeScanner]) {
    new cip = CodeScanGetMatchAddressData(match);
    new op_type = CodeScanGetMatchHole(match, 0);
    
    CodeScanNOPMatch(match);
    
    new ctx[AsmContext];
    AsmInitPtr(ctx, cip, 24);
    
    // Reemplazar múltiples operaciones por versiones optimizadas
    switch(op_type) {
        case 1: { // Multiplicación por potencia de 2
            @emit shl.c.pri 1  // Reemplaza * 2 con << 1
        }
        case 2: { // División por potencia de 2
            @emit shr.c.pri 1  // Reemplaza / 2 con >> 1
        }
        case 3: { // Módulo por potencia de 2
            @emit const.alt 1
            @emit and          // Reemplaza % 2 con & 1
        }
    }
    
    optimizations_count[3]++;
    return 1;
}

// 5. Eliminación de código muerto
static stock RemoveDeadCode(const match[CodeScanner]) {
    CodeScanNOPMatch(match);
    optimizations_count[4]++;
    return 1;
}

// Función principal de FillMemory que reemplazamos
FillMemory(arr[], val = 0, size = sizeof(arr)) {
    #pragma unused arr, val, size
    return 0;
}

// Función principal de MemoryCopy que reemplazamos
MemoryCopy(dest[], const source[], size = sizeof(source)) {
    #pragma unused dest, source, size
    return 0;
}

// Iniciar proceso de optimización
static ApplyOptimizations() {
    if (s_OptimizationsApplied) return 0;
    
    new scanner[CodeScanner];
    CodeScanInit(scanner);
    
    // Patrón 1: Optimizar FillMemory
    new matcher_fillmem[CodeScanMatcher];
    CodeScanMatcherInit(matcher_fillmem, &OptimizeFillMemory);
    CodeScanMatcherPattern(matcher_fillmem,
        OP(PUSH_C, ???)
        OP(PUSH_C, ???)
        OP(PUSH_ADR, ???)
        OP(PUSH_C, 0xc)
        OP(CALL, __addressof(FillMemory))
    );
    CodeScanAddMatcher(scanner, matcher_fillmem);
    
    // Patrones adicionales para FillMemory con diferentes formas
    new matcher_fillmem2[CodeScanMatcher];
    CodeScanMatcherInit(matcher_fillmem2, &OptimizeFillMemory);
    CodeScanMatcherPattern(matcher_fillmem2,
        OP(CONST_PRI, ???)
        OP(PUSH_PRI)
        OP(CONST_PRI, ???)
        OP(PUSH_PRI)
        OP(ADDR_PRI, ???)
        OP(PUSH_PRI)
        OP(PUSH_C, 0xc)
        OP(CALL, __addressof(FillMemory))
    );
    CodeScanAddMatcher(scanner, matcher_fillmem2);
    
    // Patrón 2: Optimizar MemoryCopy
    new matcher_memcopy[CodeScanMatcher];
    CodeScanMatcherInit(matcher_memcopy, &OptimizeMemoryCopy);
    CodeScanMatcherPattern(matcher_memcopy,
        OP(PUSH_C, ???)
        OP(PUSH_ADR, ???)
        OP(PUSH_ADR, ???)
        OP(PUSH_C, 0xc)
        OP(CALL, __addressof(MemoryCopy))
    );
    CodeScanAddMatcher(scanner, matcher_memcopy);
    
    // Patrón 3: Optimizar bucles simples
    new matcher_loops[CodeScanMatcher];
    CodeScanMatcherInit(matcher_loops, &OptimizeSimpleLoop);
    CodeScanMatcherPattern(matcher_loops,
        OP(CONST_PRI, ???)  // loop start
        OP(STOR_S, ???)     // iterator
        OP(LOAD_S, ???)     // load iterator  
        OP(CONST_ALT, ???)  // loop end
        OP(JSLESS, _)       // jump if less
        OP(JUMP, _)         // end loop jump
        OP(ADDR_PRI, ???)   // array address
    );
    CodeScanAddMatcher(scanner, matcher_loops);
    
    // Patrón 4: Optimizar operaciones matemáticas
    new matcher_math[CodeScanMatcher];
    CodeScanMatcherInit(matcher_math, &OptimizeMathOperations);
    CodeScanMatcherPattern(matcher_math,
        OP(CONST_ALT, 2)    // Cargar constante 2
        OP(MUL, 0)          // Multiplicar
    );
    CodeScanAddMatcher(scanner, matcher_math);
    
    // Patrón 5: Eliminar código muerto (código que nunca se ejecuta)
    new matcher_deadcode[CodeScanMatcher];
    CodeScanMatcherInit(matcher_deadcode, &RemoveDeadCode);
    CodeScanMatcherPattern(matcher_deadcode,
        OP(JUMP, _)         // Salto incondicional
        OP(CONST_PRI, _)    // Código que nunca se ejecuta
        OP(STOR_S, _)
    );
    CodeScanAddMatcher(scanner, matcher_deadcode);
    
    // Ejecutar escaneo y aplicar optimizaciones
    CodeScanRun(scanner);
    
    // Marcar como aplicado
    s_OptimizationsApplied = true;
    return 1;
}

// Ganchos para eventos del juego
public OnJITCompile() {
    ApplyOptimizations();
    
    #if defined ADV_OPT_OnJITCompile
        return ADV_OPT_OnJITCompile();
    #else
        return 1;
    #endif
}

#if defined _ALS_OnJITCompile
    #undef OnJITCompile
#else
    #define _ALS_OnJITCompile
#endif
#define OnJITCompile ADV_OPT_OnJITCompile
#if defined ADV_OPT_OnJITCompile
    forward ADV_OPT_OnJITCompile();
#endif

public OnGameModeInit() {
    if (!s_OptimizationsApplied) {
        ApplyOptimizations();
    }
    
    // Mostrar estadísticas de optimización
    printf("[Optimizer] Optimizaciones aplicadas: FillMemory=%d, MemoryCopy=%d, Loops=%d, Math=%d, DeadCode=%d",
        optimizations_count[0], optimizations_count[1], optimizations_count[2], 
        optimizations_count[3], optimizations_count[4]);
    
    #if defined ADV_OPT_OnGameModeInit
        return ADV_OPT_OnGameModeInit();
    #else
        return 1;
    #endif
}

#if defined _ALS_OnGameModeInit
    #undef OnGameModeInit
#else
    #define _ALS_OnGameModeInit
#endif
#define OnGameModeInit ADV_OPT_OnGameModeInit
#if defined ADV_OPT_OnGameModeInit
    forward ADV_OPT_OnGameModeInit();
#endif