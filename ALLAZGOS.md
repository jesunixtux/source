# Hallazgos de ingeniería inversa de Portal 2

> Documento de investigación para la rama `codex/portal2-compat`.
>
> Objetivo: registrar evidencia obtenida del Portal 2 original y compararla con el Source actual sin confundir hechos observados con inferencias.

---

# 1. Objetivo del análisis

El propósito de este trabajo **no es reconstruir el código fuente original de Valve**.

El flujo utilizado es:

```text
Portal 2 original
        ↓
server.dll / client.dll
        ↓
Ghidra + REA/OpenCode
        ↓
RTTI / strings / XREFs / call graph / pseudocódigo
        ↓
comprender comportamiento
        ↓
comparar con nuestro Source
        ↓
implementar una solución propia y mantenible
```

La prioridad es analizar únicamente los sistemas necesarios para hacer funcionar correctamente Portal 2 sobre este Source.

La metodología debe ser:

```text
fallo real
↓
identificar subsistema
↓
obtener evidencia del retail
↓
comparar con nuestro Source
↓
implementar solo lo necesario
↓
compilar
↓
probar
```

No se pretende descompilar indiscriminadamente Portal 2 completo.

---

# 2. server.dll analizado

Binario:

```text
Portal 2/portal2/bin/server.dll
```

Formato observado:

```text
PE DLL
x86 / 32-bit
~8.9 MB
```

SHA-256 observado:

```text
deb9fc303fdf4b0b0edcf58c4e1b17fbf20276f047728180f7ba7c8bf510f6b6
```

Exports PE observados:

```text
0x104c9540 CreateInterface
0x10530d2c entry / DllMain
```

## Importante

`SetSpeedReal` **no debe considerarse un export PE**.

El DLL solamente expone públicamente las dos entradas anteriores.

Cuando aparezca `SetSpeedReal` en el análisis se trata de lógica interna, strings, datadesc, inputs u otras estructuras recuperadas del binario, no de un símbolo exportado por PE.

---

# 3. Interfaces observadas en server.dll

Durante el análisis aparecieron strings correspondientes a varias interfaces del Source Engine.

Entre ellas:

```text
VEngineServer022
VEngineCvar007
VEngineServerStringTable001
VEngineVGui001
VEngineRandom001
VServerDllSharedAppSystems001
VFileSystem017
VNewAsyncFileSystem001
VMaterialSystem080
VMaterialSystem2_001
VPhysics031
VPhysicsSurfaceProps001
VPhysicsCollision007
VDataCache003
```

También se encontraron interfaces Steam, entre ellas:

```text
SteamClient020
SteamUser021
SteamFriends017
SteamUtils010
SteamMatchMaking009
SteamNetworking006
SteamInput006
SteamUGC_INTERFACE_VERSION016
```

Esto confirma que el binario analizado corresponde correctamente a un módulo servidor de Portal 2 basado en Source.

---

# 4. RTTI encontrado en server.dll

## CFuncTrackTrain

RTTI:

```text
.?AVCFuncTrackTrain@@
```

Ubicación observada:

```text
file offset: 0x76eca0
virtual:     0x1060FCA0
```

También aparecen clases relacionadas:

```text
CFuncTrackChange
CFuncTrackAuto
CFuncMoveLinear
CFuncConveyor
CFuncRotating
CPathTrack
```

## Advertencia importante

La existencia o proximidad de estas clases **no demuestra una relación de herencia entre ellas**.

En nuestro Source actual:

```cpp
class CFuncTrackTrain : public CBaseEntity
```

Por lo tanto, NO debe documentarse como hecho que:

```text
CFuncTrackTrain
    ↓
CFuncMoveLinear
    ↓
CBaseEntity
```

hasta demostrarlo correctamente mediante RTTI Base Class Descriptors o evidencia equivalente del binario retail.

---

## CGameMovement

RTTI observado:

```text
.?AVCGameMovement@@
```

Ubicación reportada:

```text
0x744a20
```

---

## CPortalGameMovement

RTTI observado:

```text
.?AVCPortalGameMovement@@
```

Ubicación reportada:

```text
0x78aadc
```

Esto confirma que Portal 2 contiene una implementación específica de movimiento sobre el sistema general de `CGameMovement`.

---

# 5. Strings específicas de Portal 2 encontradas

Entre las strings observadas aparecen:

```text
Portal 2: Nest
#Portal_Chapter12_Title
#Portal_Chapter9_Title
CAreaPortal
CFuncAreaPortalWindow
func_areaportal
```

También se encontró una ruta interna de compilación:

```text
D:\portal2\rel_pc\src\public\tier1\utlsortvector.h
```

Este tipo de rutas es útil como evidencia de procedencia y organización interna, pero no debe interpretarse como código fuente recuperado.

---

# 6. Ground Entity y Base Velocity en server.dll

Se localizaron las strings:

```text
m_hGroundEntity
m_vecBaseVelocity
```

Ubicaciones reportadas:

```text
m_hGroundEntity     0x5ba48c
m_vecBaseVelocity   0x5ba438
```

Estas estructuras son fundamentales para el movimiento sobre plataformas.

Conceptualmente:

```text
Player
  ↓
m_hGroundEntity
  ↓
suelo / plataforma / entidad móvil
```

Y:

```text
m_vecBaseVelocity
```

mantiene velocidad heredada de una superficie u otra entidad.

---

# 7. Mecanismo existente en nuestro Source

Nuestro Source ya contiene lógica explícita para manejar `GroundEntity` y `BaseVelocity`.

Archivo:

```text
game/shared/gamemovement.cpp
```

Código existente:

```cpp
void CGameMovement::SetGroundEntity( trace_t *pm )
{
    CBaseEntity *newGround = pm ? pm->m_pEnt : NULL;

    CBaseEntity *oldGround = player->GetGroundEntity();
    Vector vecBaseVelocity = player->GetBaseVelocity();

    if ( !oldGround && newGround )
    {
        vecBaseVelocity -= newGround->GetAbsVelocity(); 
        vecBaseVelocity.z = newGround->GetAbsVelocity().z;
    }
    else if ( oldGround && !newGround )
    {
        vecBaseVelocity += oldGround->GetAbsVelocity();
        vecBaseVelocity.z = oldGround->GetAbsVelocity().z;
    }

    player->SetBaseVelocity( vecBaseVelocity );
    player->SetGroundEntity( newGround );

    if ( newGround )
    {
        CategorizeGroundSurface( *pm );

        player->m_flWaterJumpTime = 0;

        if ( !pm->DidHitWorld() )
        {
            MoveHelper()->AddToTouched( *pm, mv->m_vecVelocity );
        }

        mv->m_vecVelocity.z = 0.0f;
    }
}
```

Esto demuestra que nuestro Source ya implementa el mecanismo básico de asociación entre:

```text
player
+
ground entity
+
base velocity
```

---

# 8. Uso de BaseVelocity durante movimiento

Nuestro `CGameMovement` también incorpora la `BaseVelocity` al movimiento efectivo del jugador.

Ejemplo existente:

```cpp
VectorAdd(
    mv->m_vecVelocity,
    player->GetBaseVelocity(),
    mv->m_vecVelocity
);

TryPlayerMove();

VectorSubtract(
    mv->m_vecVelocity,
    player->GetBaseVelocity(),
    mv->m_vecVelocity
);
```

Conceptualmente:

```text
velocidad propia del jugador
+
velocidad heredada del ground entity
=
movimiento efectivo
```

Por tanto, el sistema básico necesario para acompañar plataformas móviles **ya existe**.

---

# 9. CFuncTrackTrain en nuestro Source

Archivo:

```text
game/server/trains.cpp
```

Registro:

```cpp
LINK_ENTITY_TO_CLASS( func_tracktrain, CFuncTrackTrain );
```

Declaración actual:

```cpp
class CFuncTrackTrain : public CBaseEntity
```

---

# 10. MOVETYPE_PUSH

En `CFuncTrackTrain::Spawn()` nuestro Source utiliza:

```cpp
SetMoveType( MOVETYPE_PUSH );
```

También inicializa colisión y modelo.

Este detalle es importante porque las plataformas y trenes móviles utilizan el sistema de `PUSH` para interactuar con otras entidades.

---

# 11. SetSpeedReal en nuestro Source

El input está registrado mediante:

```cpp
DEFINE_INPUTFUNC(
    FIELD_FLOAT,
    "SetSpeedReal",
    InputSetSpeedReal
);
```

Implementación:

```cpp
void CFuncTrackTrain::InputSetSpeedReal( inputdata_t &inputdata )
{
    SetSpeed(
        clamp(
            inputdata.value.Float(),
            0.f,
            m_maxSpeed
        )
    );
}
```

El flujo actual es:

```text
SetSpeedReal
    ↓
SetSpeed()
    ↓
Start() / Next()
    ↓
UpdateTrainVelocity()
```

---

# 12. Movimiento de CFuncTrackTrain

Nuestro Source calcula la velocidad del tren en:

```cpp
CFuncTrackTrain::UpdateTrainVelocity()
```

Fragmento relevante:

```cpp
Vector velDesired = nextPos - GetLocalOrigin();

VectorNormalize( velDesired );

velDesired *= fabs( m_flSpeed );

SetLocalVelocity( velDesired );
```

Por tanto, actualmente no parece que el problema del ascensor consista simplemente en que:

```text
SetSpeedReal no existe
```

o:

```text
el tren no recibe velocidad
```

El ascensor sí logra desplazarse.

---

# 13. Path tracking

El Source también contiene lógica de seguimiento de `path_track`.

Por ejemplo:

```cpp
CFuncTrackTrain::Next()
```

utiliza:

```cpp
m_ppath->LookAhead(...)
```

y posteriormente actualiza:

```text
velocidad
orientación
path actual
outputs OnNextPoint
```

También existe la validación:

```text
func_track_train must be on a path of path_track
```

String que también fue localizada durante el análisis del binario retail.

---

# 14. Síntoma actual del ascensor

Caso de prueba observado:

```text
el jugador entra al ascensor
↓
el ascensor funciona
↓
el ascensor desciende
↓
llega a destino
↓
la puerta se abre
↓
el jugador cae varios pisos
```

Este comportamiento permite deducir que varias piezas ya funcionan.

Probablemente:

```text
map I/O                  funciona
inicio del ascensor      funciona
SetSpeedReal             funciona
movimiento del tren      funciona al menos parcialmente
detección de llegada     funciona
apertura de puerta       funciona
```

El problema parece estar relacionado con:

```text
player ↔ plataforma
```

durante o después del descenso.

---

# 15. ClientVerticalElevatorFixes en nuestro Source

En:

```text
game/shared/portal/portal_gamemovement.cpp
```

existe:

```cpp
CPortalGameMovement::ClientVerticalElevatorFixes()
```

Es llamada desde:

```cpp
CPortalGameMovement::ProcessMovement()
```

mediante:

```cpp
#if defined( CLIENT_DLL )
    ClientVerticalElevatorFixes( pPlayer, pMove );
#endif
```

Con el comentario:

```cpp
//fixup vertical elevator discrepancies between client and server as best we can
```

Esto confirma que el código disponible contiene una rutina específicamente pensada para discrepancias en ascensores verticales.

---

# 16. Código deshabilitado mediante #if 0

Dentro de:

```cpp
ClientVerticalElevatorFixes()
```

existe una parte importante protegida por:

```cpp
#if 0
```

Ese bloque intenta conceptualmente:

```text
obtener GetGroundEntity()
↓
recorrer GetMoveParent()
↓
buscar el root move parent
↓
identificar C_BaseToggle
↓
comprobar movimiento lineal
↓
manejar prediction
↓
predecir posición del mover
↓
ajustar posición Z
↓
invalidar transforms
```

Posteriormente existe código para:

```text
re-seat player on vertical elevators
```

incluyendo traces verticales y corrección de posición.

Inicialmente este bloque era uno de los principales sospechosos del bug.

El análisis de `client.dll` cambió esa hipótesis.

---

# 17. client.dll analizado

Después de `server.dll`, se analizó también el:

```text
Portal 2/portal2/bin/client.dll
```

de Portal 2.

El objetivo era verificar si el cliente retail contiene una implementación equivalente al:

```cpp
CPortalGameMovement::ClientVerticalElevatorFixes()
```

presente en nuestro Source.

---

# 18. CPortalGameMovement RTTI en client.dll

RTTI encontrado:

```text
.?AVCPortalGameMovement@@
```

Ubicación reportada:

```text
0x93f254
```

Esto confirma la presencia de `CPortalGameMovement` en el cliente retail.

---

# 19. CGameMovement RTTI en client.dll

RTTI encontrado:

```text
.?AVCGameMovement@@
```

Ubicación reportada:

```text
0x92cd38
```

Esto confirma la coexistencia de:

```text
CGameMovement
CPortalGameMovement
```

en el cliente.

---

# 20. cl_vertical_elevator_fix

Se localizó la string:

```text
cl_vertical_elevator_fix
```

Ubicación reportada:

```text
VMA:         0x107c16d0
file offset: 0x7c16d0
section:     .rdata
```

La presencia de esta string confirma que el cliente retail contiene o registra una ConVar con ese nombre.

Sin embargo:

```text
string encontrada
```

NO implica automáticamente:

```text
la funcionalidad está activa
```

ni:

```text
existe exactamente nuestro ClientVerticalElevatorFixes()
```

---

# 21. Campos relevantes encontrados en client.dll

Se reportaron referencias/netvars relacionadas con:

```text
m_hGroundEntity
m_vecBaseVelocity
m_hNetworkMoveParent
```

Ubicaciones reportadas:

```text
m_hGroundEntity       0x7463f4
m_vecBaseVelocity     0x7463ac
m_hNetworkMoveParent  0x7464d0
```

Esto respalda que el cliente dispone de la información necesaria para conocer:

```text
ground entity
base velocity
move parent
```

---

# 22. Entidades lineales observadas en client.dll

También aparecieron strings o estructuras relacionadas con:

```text
func_movelinear
func_door
C_BaseToggle
```

Esto es consistente con la infraestructura de movimiento esperada.

Sin embargo, la existencia de estos elementos no demuestra todavía que sean utilizados específicamente por la lógica del ascensor estudiado.

---

# 23. ClientVerticalElevatorFixes retail no confirmado

El análisis estático de `client.dll` **no encontró una función claramente identificable** que reproduzca íntegramente el comportamiento del código de nuestro Source.

No se confirmó estáticamente:

```text
predicción específica de Z del ground entity
ajuste explícito del root Z
invalidación explícita de abs transforms
trace vertical específico del elevator
re-seat específico del player
actualización específica de NetworkOrigin
```

Por tanto:

```text
ClientVerticalElevatorFixes retail
```

permanece:

```text
NO CONFIRMADO
```

---

# 24. Posibles explicaciones

Actualmente hay varias posibilidades.

## Posibilidad A

El bloque protegido con:

```cpp
#if 0
```

también estaba deshabilitado en el Portal 2 retail.

Podría ser:

```text
código experimental
código abandonado
código de desarrollo
intento de fix finalmente descartado
```

---

## Posibilidad B

La lógica existe, pero fue integrada dentro de otra función, posiblemente:

```text
CPortalGameMovement::ProcessMovement()
```

y por eso no aparece como una rutina independiente.

---

## Posibilidad C

El Portal 2 retail depende principalmente del mecanismo estándar:

```text
GroundEntity
+
BaseVelocity
+
MOVETYPE_PUSH
```

y la corrección especial del bloque deshabilitado normalmente no es necesaria.

Esta posibilidad gana importancia después del análisis actual.

---

# 25. Cambio de hipótesis principal

Hipótesis anterior:

```text
principal sospechoso:
ClientVerticalElevatorFixes parcialmente deshabilitado
```

Hipótesis actual:

```text
principal sospechoso:
GroundEntity / BaseVelocity / contacto con el ascensor
```

El código bajo `#if 0` **NO debe reactivarse a ciegas**.

No tenemos evidencia suficiente para afirmar que el retail lo utilice.

---

# 26. Nueva hipótesis del bug

El flujo esperado es:

```text
CFuncTrackTrain
      ↓
MOVETYPE_PUSH
      ↓
player entra en contacto con la superficie
      ↓
SetGroundEntity(elevator)
      ↓
m_hGroundEntity = elevator
      ↓
GetAbsVelocity(elevator)
      ↓
m_vecBaseVelocity
      ↓
GameMovement incorpora BaseVelocity
      ↓
player acompaña al elevator
```

El fallo puede encontrarse en alguno de estos puntos.

---

# 27. Posibles fallos actuales

## 1. GroundEntity se pierde

Durante el descenso puede ocurrir:

```text
GetGroundEntity()
```

pase de:

```text
arrival_elevator-elevator_1
```

a:

```text
NULL
```

antes de tiempo.

---

## 2. BaseVelocity incorrecta

Puede ocurrir que:

```text
train velocity Z = -300
```

pero:

```text
player BaseVelocity Z = 0
```

o tenga un valor incorrecto.

---

## 3. Colisión del piso

El jugador puede dejar de considerar el piso interior como una superficie válida durante el movimiento.

Esto puede estar relacionado con:

```text
SOLID_VPHYSICS
MOVETYPE_PUSH
collision hierarchy
player traces
```

---

## 4. Diferencia client/server

El servidor puede considerar:

```text
player grounded
```

mientras el cliente predice:

```text
player airborne
```

o viceversa.

---

## 5. Ground entity hierarchy

El jugador puede estar realmente apoyado sobre:

```text
child entity
```

de la estructura del elevator en lugar de directamente sobre:

```text
CFuncTrackTrain
```

y el hierarchy puede no estar siendo tratado correctamente.

---

## 6. Portal-specific movement

Puede existir una diferencia todavía no reconstruida dentro de:

```text
CPortalGameMovement
```

relacionada con ascensores, traces o prediction.

---

# 28. CPortalGameMovement vtable reportada

OpenCode reportó una posible vtable alrededor de:

```text
0x10a09fe8
```

con cientos de entradas y valores aproximadamente:

```text
0x3478
0x347c
0x3480
0x3484
...
```

Esto es sospechoso.

No debe considerarse confirmado que se trate de una vtable real de más de 300 métodos.

Podría tratarse de:

```text
tabla de offsets
jump table
estructura auxiliar
datos mal interpretados
```

---

# 29. Cómo verificar correctamente la vtable

Debe seguirse:

```text
RTTI Complete Object Locator
↓
Class Hierarchy Descriptor
↓
Base Class Descriptors
↓
vftable references
↓
funciones
```

Solo después se pueden asignar correctamente métodos a índices.

---

# 30. ProcessMovement no identificado todavía

Se reportaron como posibles candidatos:

```text
0x10003480
0x10003484
```

Pero actualmente no existe evidencia suficiente para afirmar que cualquiera sea:

```cpp
CPortalGameMovement::ProcessMovement()
```

El hecho de que Ghidra pueda decompilar una dirección con un prólogo x86 válido **no demuestra la identidad de la función**.

---

# 31. Identificación correcta de ProcessMovement

Debe utilizarse:

```text
RTTI/vtable verificada
callers
callees
CBasePlayer*
CMoveData*
accesos a player
accesos a movement
comparación con CGameMovement
```

y solo entonces asignar:

```text
CPortalGameMovement::ProcessMovement
```

a una función.

---

# 32. cl_vertical_elevator_fix necesita XREF real

Actualmente tenemos:

```text
string                               CONFIRMADA
existencia probable del ConVar       ALTA CONFIANZA
función que consulta su valor         NO CONFIRMADA
GetBool/GetInt/GetFloat               NO CONFIRMADO
```

Debe localizarse el objeto real del ConVar y buscar:

```text
constructor
registro
XREFs al objeto
GetBool
GetInt
GetFloat
```

Si no aparecen lecturas del ConVar después de su registro, eso sería evidencia importante de que el fix quedó inactivo en retail.

---

# 33. m_vecBaseVelocity no demuestra NetworkOrigin

Encontrar:

```text
m_vecBaseVelocity
```

no demuestra automáticamente una relación con:

```text
SetNetworkOrigin
```

o cualquier corrección concreta de posición.

Cada relación debe obtenerse mediante evidencia real de llamadas o accesos.

---

# 34. String de GameMovement

Se encontró:

```text
PORTALLING PLAYER SHOULD BE DONE IN GAMEMOVEMENT
```

Esto constituye evidencia adicional de que Portal 2 concentra parte importante de la lógica de portales/player dentro del sistema de GameMovement.

No permite por sí sola identificar una función concreta.

---

# 35. Próxima investigación en client.dll

Prioridad:

```text
identificar correctamente CPortalGameMovement
↓
verificar su vtable
↓
identificar ProcessMovement
↓
decompilar ProcessMovement
```

Dentro de esa función buscar:

```text
m_hGroundEntity
m_vecBaseVelocity
GetMoveParent
origin
Z position
player ground traces
cl_vertical_elevator_fix
```

---

# 36. Próxima investigación en server.dll

Se debe localizar el equivalente retail de:

```cpp
CGameMovement::SetGroundEntity()
```

Utilizando como anclas:

```text
m_hGroundEntity
m_vecBaseVelocity
GetAbsVelocity
```

Interesan especialmente los casos:

```text
oldGround == NULL
newGround != NULL
```

y:

```text
oldGround != NULL
newGround == NULL
```

---

# 37. Qué queremos confirmar en SetGroundEntity retail

Nuestro Source actualmente hace:

```cpp
if ( !oldGround && newGround )
{
    vecBaseVelocity -= newGround->GetAbsVelocity(); 
    vecBaseVelocity.z = newGround->GetAbsVelocity().z;
}
```

y:

```cpp
else if ( oldGround && !newGround )
{
    vecBaseVelocity += oldGround->GetAbsVelocity();
    vecBaseVelocity.z = oldGround->GetAbsVelocity().z;
}
```

Queremos saber si Portal 2 retail hace realmente algo equivalente.

El Source disponible debe utilizarse **solo para comparar después**, no como prueba para nombrar las funciones del binario.

---

# 38. Instrumentación recomendada

Antes de modificar comportamiento, conviene instrumentar el port actual.

Registrar durante el ascensor:

```text
GetGroundEntity()
GetGroundEntity()->GetClassname()
GetGroundEntity()->GetAbsOrigin()
GetGroundEntity()->GetAbsVelocity()
player->GetBaseVelocity()
player origin
player velocity
train origin
train velocity
FL_ONGROUND
move type
tick/time
```

---

# 39. Ejemplo esperado de log correcto

```text
tick 100
ground=arrival_elevator-elevator_1
train_vel_z=-300
base_vel_z=-300
player_z=1024

tick 101
ground=arrival_elevator-elevator_1
train_vel_z=-300
base_vel_z=-300
player_z=1019

tick 102
ground=arrival_elevator-elevator_1
train_vel_z=-300
base_vel_z=-300
player_z=1014
```

---

# 40. Ejemplo de fallo que queremos detectar

```text
tick 100
ground=arrival_elevator-elevator_1
train_vel_z=-300
base_vel_z=-300

tick 101
ground=arrival_elevator-elevator_1
train_vel_z=-300
base_vel_z=-300

tick 102
ground=NULL
train_vel_z=-300
base_vel_z=0

tick 103
ground=NULL
player_vel_z=-80

tick 104
player falling
```

Si aparece algo similar, habremos localizado el momento exacto del fallo.

---

# 41. Código de llegada del elevator en nuestro port

Actualmente existe lógica especial para los elevators de los mapas intro.

El runtime busca:

```text
@arrival_teleport
arrival_elevator-elevator_1
@elevator_1_bottom_path_1
```

Luego teleporta al jugador y activa:

```text
SetSpeedReal
```

sobre el train.

El propio runtime contiene una comprobación:

```text
PORTAL2_ARRIVAL ride=PASS
```

o:

```text
PORTAL2_ARRIVAL ride=FAIL
```

Esto es muy útil como test de integración.

---

# 42. Condición actual de ride test

El código evalúa aproximadamente:

```cpp
const bool carried =
    m_startZ - pos.z > 600.0f &&
    (pos - m_train->GetAbsOrigin()).Length2D() < 100.0f;
```

Por tanto el propio port ya dispone de una prueba para saber si el ascensor transportó realmente al jugador durante el descenso.

Este resultado debe registrarse durante futuras pruebas.

---

# 43. Qué sabemos que funciona actualmente

```text
server.dll puede analizarse con Ghidra
client.dll puede analizarse con Ghidra

CFuncTrackTrain existe
CGameMovement existe
CPortalGameMovement existe

SetSpeedReal existe en nuestro Source
MOVETYPE_PUSH existe
GroundEntity existe
BaseVelocity existe

el ascensor se mueve
el ascensor llega
la puerta abre
```

---

# 44. Qué todavía no sabemos

```text
por qué exactamente cae el jugador

en qué frame GroundEntity cambia

si GroundEntity realmente cambia

si BaseVelocity Z es incorrecta

si el interior del elevator tiene colisión incorrecta

si existe una discrepancia de prediction

si el problema es server-side

si el problema es client-side

si hay lógica retail adicional todavía no identificada

si cl_vertical_elevator_fix está realmente activo

si el #if 0 también existe funcionalmente en retail
```

---

# 45. Hipótesis priorizadas actualmente

Orden aproximado:

```text
1. GroundEntity se pierde durante el movimiento vertical.

2. BaseVelocity Z no se propaga correctamente.

3. El piso/colisión del elevator deja de considerarse ground.

4. Existe una divergencia client/server durante prediction.

5. GroundEntity apunta a una entidad hija/hierarchy inesperada.

6. Falta algún comportamiento específico de CPortalGameMovement.

7. El bloque #if 0 de ClientVerticalElevatorFixes es necesario.
```

La hipótesis 7 pasó de ser una de las primeras a una hipótesis secundaria.

---

# 46. Regla de investigación

Cada hallazgo debe clasificarse mediante uno de estos estados:

```text
OBSERVADO EN BINARIO

CONFIRMADO POR PSEUDOCÓDIGO

CONFIRMADO POR ASSEMBLY

INFERIDO CON ALTA CONFIANZA

NO DETERMINADO
```

No convertir una inferencia basada en conocimiento de Source en un hecho observado.

---

# 47. Ejemplo de inferencia incorrecta

Incorrecto:

```text
Encontramos m_hGroundEntity.

Por lo tanto FUN_1234 es CBaseEntity::SetGroundEntity.
```

Esto no constituye evidencia suficiente.

---

# 48. Ejemplo correcto

Correcto:

```text
FUN_1234 accede al campo relacionado con m_hGroundEntity.

También modifica un EHANDLE.

La función es llamada desde una ruta de ground categorization.

Esto hace probable que sea SetGroundEntity,
pero todavía necesita confirmación mediante callers/callees,
assembly o comparación estructural.
```

---

# 49. Otro error que debe evitarse

No debe afirmarse:

```text
CFuncTrackTrain tiene ocho métodos virtuales,
por lo tanto vtable[0] es VPhysicsUpdate.
```

La posición de una función en una tabla no basta para nombrarla.

Debe investigarse:

```text
callers
callees
RTTI
parámetros
campos accedidos
strings
assembly
```

---

# 50. Estado de VScript

Nuestro port ya contiene una implementación real de VScript/Squirrel.

Archivo relevante:

```text
game/server/portal2/portal2_vscript.cpp
```

La implementación actual incluye:

```text
VM Squirrel real
RunCode
RunFile
CallFunction
per-entity scopes
thisEntity
```

Bindings conocidos:

```text
Time
FrameTime
Vector
EntFire
EntFireByHandle
IncludeScript
Entities.FindByName
Entities.FindByClassname
Entities.First
Entities.Next
```

La estrategia futura debe ser:

```text
map ejecuta script
↓
Missing native function: X
↓
buscar X en retail
↓
XREFs
↓
función
↓
callers/callees
↓
comprender contrato
↓
implementar solo X
```

No intentar reconstruir todo VScript de una vez.

---

# 51. Portal placement

Nuestro Source contiene implementación compartida de portal placement, pero todavía existen compatibility stubs en algunas rutas.

Ejemplo de client compat:

```text
portal_placement_compat.cpp
```

Algunos métodos actualmente devuelven resultados artificiales como:

```text
SUCCESS
false
0
```

Esto requerirá investigación posterior cuando cause fallos reales.

---

# 52. Grab controller

Existe una implementación grande en:

```text
game/shared/portal2/portal_grabcontroller_shared.cpp
```

También existen compatibility stubs.

Antes de reversear el retail debe comprobarse primero que la implementación real esté correctamente compilada y enlazada en el target Portal 2.

---

# 53. Player pickup

Existen:

```text
game/shared/portal2/player_pickup.cpp
game/shared/portal2/player_pickup.h
game/server/portal2/player_pickup_controller.cpp
```

Sin embargo, parte de esta infraestructura puede no estar correctamente incluida en los VPC utilizados por WAF.

Prioridad:

```text
verificar build wiring
antes de reverse engineering
```

---

# 54. Paint

El Source contiene bastante infraestructura real de paint:

```text
paint_blobs_shared.cpp
paint_cleanser
paint_color
paint_power
paint_sprayer
paint_stream
```

Pero también existen funciones server compat vacías, incluyendo operaciones relacionadas con actualización de blobs y colisión.

Estas deberán investigarse cuando sean bloqueo real de mapas.

---

# 55. Tractor beam

Existe:

```text
trigger_tractorbeam_shared.cpp
trigger_tractorbeam_shared.h
```

También existen compat functions vacías del servidor.

No es prioridad mientras el objetivo principal siga siendo terminar correctamente el Single Player inicial.

---

# 56. Multiplayer / coop

Existen varios compatibility stubs relacionados con:

```text
Portal MP gamerules
VS
coop
stats
```

Actualmente son prioridad baja.

La meta principal continúa siendo:

```text
Single Player
```

Por tanto, no deben consumirse recursos de reverse engineering en multiplayer salvo que bloqueen accidentalmente el SP.

---

# 57. Instructor / movies

Existen implementations parciales o compat de:

```text
instructor
lessons
movies
```

No constituyen actualmente un bloqueo principal de campaña.

---

# 58. Partículas y materiales

El port tiene limitaciones independientes del código gameplay.

Entre ellas:

```text
PCF/DMX de Portal 2
materiales
algunos efectos
HDR cubemaps
```

Estas limitaciones deben mantenerse separadas de los fallos de gameplay.

---

# 59. Estado general del enfoque de reverse engineering

El proyecto ya dejó atrás la fase:

```text
"¿Podemos siquiera analizar server.dll?"
```

Actualmente sabemos que:

```text
Ghidra 12.1.4
+
WSL
+
REA/OpenCode
```

pueden analizar correctamente el binario x86 de Portal 2.

Por tanto, el nuevo enfoque debe ser:

```text
problema específico
↓
función específica
↓
evidencia específica
```

---

# 60. Siguiente objetivo inmediato

Para el bug del ascensor:

```text
1. Instrumentar GroundEntity/BaseVelocity.

2. Reproducir el descenso.

3. Obtener PORTAL2_ARRIVAL ride=PASS/FAIL.

4. Registrar el primer frame donde el comportamiento diverge.

5. Si GroundEntity se pierde:
   investigar CategorizePosition / ground trace / collision.

6. Si GroundEntity permanece pero BaseVelocity falla:
   investigar SetGroundEntity / BaseVelocity.

7. Si servidor funciona pero cliente diverge:
   investigar CPortalGameMovement / prediction.

8. Solo después diseñar el parche.
```

---

# 61. Próximo objetivo de Ghidra

En `client.dll`:

```text
verificar CPortalGameMovement vtable
↓
identificar ProcessMovement
↓
decompilar ruta relevante
↓
buscar uso real de cl_vertical_elevator_fix
```

En `server.dll`:

```text
identificar SetGroundEntity
↓
ver comportamiento exacto de BaseVelocity
↓
comparar con nuestro Source
```

---

# 62. Caso de aceptación del ascensor

El comportamiento correcto debe ser:

```text
Player entra al elevator
↓
GroundEntity = elevator
↓
elevator comienza a bajar
↓
BaseVelocity acompaña el movimiento
↓
player permanece apoyado
↓
elevator llega abajo
↓
velocidad se hace cero
↓
GroundEntity sigue válido
↓
puerta abre
↓
player puede caminar fuera
```

No:

```text
elevator baja
↓
player pierde ground
↓
gravedad
↓
puerta abre
↓
player cae dos pisos
```

---

# 63. Prioridad de subsistemas después del ascensor

Orden sugerido:

```text
1. Elevators / moving ground

2. VScript / Squirrel bindings

3. Portal 2 prediction

4. Portal placement

5. Grab controller

6. Player pickup

7. Map I/O faltante

8. Paint

9. Tractor beams

10. Instructor / presentation systems

11. Coop / multiplayer
```

El orden puede cambiar según qué mapa se convierta en el siguiente bloqueo real.

---

# 64. Principio general

No reconstruir sistemas completos porque “Portal 2 los tiene”.

Solo reconstruir aquello que:

```text
un mapa
un script
una entidad
una mecánica
```

demuestre que necesitamos.

Ejemplo:

```text
Missing native function: SomeFunction
```

es una buena razón para investigar `SomeFunction`.

La mera existencia de 500 funciones desconocidas dentro del DLL no lo es.

---

# 65. Estado actual del conocimiento del ascensor

Resumen:

```text
CFuncTrackTrain             CONFIRMADO
MOVETYPE_PUSH               CONFIRMADO EN SOURCE
SetSpeedReal                CONFIRMADO EN SOURCE
GroundEntity                CONFIRMADO
BaseVelocity                CONFIRMADO
CPortalGameMovement         CONFIRMADO
cl_vertical_elevator_fix    CONFIRMADO COMO STRING/CONVAR
ClientVerticalElevatorFixes EXISTE EN SOURCE
equivalente retail          NO CONFIRMADO
causa exacta de caída       NO DETERMINADA
```

La hipótesis más prometedora actualmente es:

```text
el player pierde o interpreta incorrectamente
su ground entity / base velocity
durante el descenso del elevator
```

---

# 66. Conclusión actual

El trabajo de reverse engineering ya permitió reducir considerablemente el área de búsqueda.

Antes:

```text
"El elevator está roto."
```

Ahora:

```text
El elevator se mueve y completa su secuencia.

El problema parece estar en la relación
entre el jugador y una superficie móvil vertical.

El mecanismo GroundEntity/BaseVelocity existe
tanto en la arquitectura observada como en nuestro Source.

La corrección especial ClientVerticalElevatorFixes
existe en nuestro código, pero parte está deshabilitada
y todavía no se ha demostrado que retail utilice esa ruta.

La siguiente acción correcta es observar el comportamiento
runtime de GroundEntity/BaseVelocity y compararlo
con las funciones retail relevantes.
```

Esto permite continuar la reconstrucción de Portal 2 de forma dirigida, verificable y mantenible.

---

# 63. Verificación estática inicial de Paint en el retail

## Artefacto y límites

Se abrió con Ghidra 12.1.4 el módulo servidor de esta instalación:

```text
Portal 2/portal2/bin/server.dll
PE32, x86
SHA-256: deb9fc303fdf4b0b0edcf58c4e1b17fbf20276f047728180f7ba7c8bf510f6b6
```

El análisis reportó 28.237 procedimientos y 23.594 strings definidas. El binario referencia `server.pdb` (GUID `06a4e8fb-8063-4e4c-9793-e37c92a1e1f6`, edad 13), pero no se encontró un PDB en la instalación inspeccionada. Por ello, Ghidra asigna nombres `FUN_<dirección>` a código no identificado; una búsqueda de nombres sin resultados no demuestra que la lógica no exista.

## Evidencia positiva

En strings y RTTI del servidor aparecen, entre otros:

```text
CPaintPowerPickup
PaintPowerInfo_t
PropPaintPowerUser
m_iPaintPower
m_nPaintPowerType
PaintPower
.?AV?$PaintPowerUser@V?$CPaintableEntity@VCBaseMultiplayerPlayer@@@@@@
.?AV?$PaintPowerUser@V?$CPaintableEntity@VCPhysicsProp@@@@@@
.?AV?$PaintPowerUser@V?$CPaintableEntity@VCNPC_FloorTurret@@@@@@
```

También se observaron nombres de entidades `weapon_portalgun`, `func_portal_bumper`, `func_portal_detector`, `func_portal_orientation` y `func_portalled`, además de cadenas y comandos del runtime `VScript`. Esto confirma que el módulo retail contiene estructuras específicas para esos sistemas; los nombres por sí solos no revelan sus algoritmos ni prueban qué rutas se ejecutan en cada mapa.

Se decompiló `FUN_103992f0` (dirección de entrada `0x103992f0`, cuerpo de 112 bytes). El pseudocódigo inicializa el descriptor asociado a `CPaintPowerPickup`; **no** es la lógica de aplicación/selección de pintura. Se conserva la dirección y el hash como localizadores reproducibles, no el pseudocódigo como si fuese fuente original.

## Comparación con el árbol actual

El árbol ya define tipos y parte de la infraestructura compartida de pintura, pero tiene huecos concretos:

- `game/server/portal2/paint_database.cpp`: `CPaintDatabase::RemoveAllPaint()` es un no-op.
- `game/server/portal/weapon_portalgun_server_compat.cpp`: `PaintPowerPickup()` y `WallPainted()` son no-op.
- `game/server/portal/paint_blobs_server_compat.cpp`: `UpdateBlobCollision()`, `UpdateBlobPostCollision()` y `PaintBlobUpdate()` son no-op.
- `game/server/portal/paint_stream.h`: `AddPaintToDatabase()` y `UpdateBlobSharedData()` son no-op.

**Conclusión:** Paint es una brecha verificable entre la estructura observada en el retail y este port, y parece un buen candidato para una investigación dirigida si un mapa de la campaña inicial lo bloquea. No se debe rellenar esos métodos deduciendo comportamiento únicamente de nombres de clase: el siguiente paso es localizar una ruta retail concreta (referencias, función y callers), contrastarla con las interfaces del port y diseñar una prueba pequeña antes del cambio.

## Evidencia que falta

- Identificar y decompilar las rutinas retail que registran, aplican, persisten y eliminan pintura; los nombres RTTI hallados no bastan.
- Determinar qué huecos anteriores están en el camino de ejecución de los mapas Single Player prioritarios.
- Comparar resultados del retail y del port en el mismo mapa, condiciones y pasos reproducibles.

No se modificó gameplay en esta pasada: todavía no hay una rutina/escenario de aceptación específico que justifique un parche de pintura.

---

# 64. Ruta de Paint en retail y reproducción en `sp_a3_portal_intro`

## Mapa de reproducción

Se inspeccionó el lump de entidades del mapa original:

```text
portal2/maps/sp_a3_portal_intro.bsp
SHA-256: b84c7a9cae3dba97761c8b496733bd0efe1c7d0799498ec0470b9694e0d203c6
Tamaño: 64.467.344 bytes
```

Contiene **22 `info_paint_sprayer`**, `trigger_paint_cleanser` y secuencias de mapa que usan `Start`/`Stop`; las propiedades incluyen `PaintType`, `blobs_per_second`, velocidades, spread, streaking y `start_active`. Es un mapa Single Player concreto para verificar creación y colisión de pintura, no una prueba de campaña completa.

## Funciones retail recuperadas

El binario sigue siendo PE32/x86 y no tiene su PDB local. Por eso, los nombres `FUN_...` son direcciones de Ghidra; las etiquetas de VProf, strings y relaciones permiten asociar varias rutinas con métodos, pero no recuperar sus nombres originales con certeza.

| Dirección en `server.dll` | Función/rol observado o inferido | Evidencia útil |
|---|---|---|
| `0x103a48e0` | Spawn de `CPaintSprayer` (inferencia fuerte) | Crea `paint_stream` y programa `PaintSprayerThink`. |
| `0x103a4030` | `PaintSprayerThink` | Registra ese nombre y llama la ruta de spray. |
| `0x103a5480` | `SprayPaint` (inferencia por callers y flujo) | Respeta intervalo/capacidad y llama al creador del blob. |
| `0x103a4e00` | Creación y configuración del blob (inferencia) | Calcula spread/velocidad, pide blob al pool y lo inicializa. |
| `0x103a6500` | Añade el blob al stream (inferencia por el cuerpo) | Inserta el puntero en el vector del stream. |
| `0x103a84d0` | `CPaintStreamManager::PaintStreamUpdate` | Etiqueta VProf explícita; recoge streams y llama al actualizador de blobs. |
| `0x1039c030` | `CBasePaintBlob::CheckCollision` | Etiqueta VProf; trata portales, tractor beam y cleanser. |
| `0x1039e6b0` | `CBasePaintBlob::ResolveCollision` | Etiqueta VProf; resuelve portal, beam, cleanser y superficies sólidas. |
| `0x103a2ba0` / `0x103a2960` | Aplicación a pared proyectada o `IPaintableEntity` (inferido) | El primero elige la ruta; el segundo llama `Paint` y emite `PaintEntity`. |
| `0x103a1d20` / `0x103a2180` | Borrar pintura de entidad / borrar toda la pintura | Limpia entidades rastreadas y la ruta global emite `RemoveAllPaint`. |

La arquitectura de blobs se apoya en `CPaintStream`; el binario también contiene RTTI de `m_sharedBlobData` y mensajes `PaintWorld`, `PaintEntity`, `RemovePaint` y `RemoveAllPaint`. El pseudocódigo descompilado no se copió al árbol.

## Diferencias encontradas en el port

- Antes de este cambio, el mapa cargaba 22 aspersores pero la sonda midió `configured=0`, `with_stream=0`, `streams=0`, `blobs=0`: `info_paint_sprayer` se registraba sin datadesc, spawn ni think.
- Este cambio añade al target Portal 2 la lectura de propiedades del mapa, creación del stream, `Start`/`Stop`/`ChangePaintType`, think con el contexto `PaintSprayerThink` y pool dimensionado con los streams creados al terminar el spawn. También registra `trigger_paint_cleanser` como trigger y libera los blobs de un stream al eliminarlo. Los streams se mantienen fuera de la red: no existe aquí la clase cliente correspondiente.
- En la ejecución aislada de `sp_a3_portal_intro`, el servidor activó el mapa y la sonda observó `sprayers=22 configured=22 with_stream=22 streams=22 cleansers=1 blobs=609`. El mapa ejecutó `Start`/`Stop`; antes del cambio los 22 aspersores tenían `PaintType=NO_POWER`, tasa por defecto y ningún stream. Esto demuestra inicialización y emisión de blobs, **no** su movimiento ni depósito.
- `PaintBlobUpdate`, `UpdateBlobCollision` y `UpdateBlobPostCollision` siguen siendo no-op en `paint_blobs_server_compat.cpp`; `CPaintBlob::PaintBlobPaint` también es no-op. La sonda capturó `PORTAL2_PAINT_STUB update=PaintBlobUpdate blobs=1 action=no-op`: el lote llega al primer punto sin implementación y no alcanza las rutas de colisión/aplicación.
- `CPaintStream::AddPaintToDatabase()` y `UpdateBlobSharedData()` siguen vacíos; `CPaintDatabase::RemoveAllPaint()` y `WallPainted()` también.
- Bloqueador de capa motor: en `engine/vengineserver_impl.cpp`, `HasPaintmap()` devuelve `false` y `SpherePaintSurface`, `SphereTracePaintSurface`, `RemoveAllPaint`, `PaintAllSurfaces` y `RemovePaint` no hacen nada. `PaintPowerUser::UpdatePaintPowers()` omite su actualización cuando no hay paintmap. No hay una reparación de superficie mundial implementada en esta pasada.

## Instrumentación y pruebas

`portal2_paint_probe` ejecutado como `--paint-probe` en `scripts/test-portal2-render.py` informa cada aspersor, configuración, stream, blobs y actividad de `Think`; solo acepta `sp_a3_portal_intro`. Su estado `PASS` comprueba el arranque de la sonda, no compatibilidad de Paint.

La compilación ARM64 completa de Portal 2 (`./scripts/build-macos-arm64.sh portal2`) y el build incremental del cambio actual terminaron correctamente. El stage indicado en `$HOME/Library/Application Support/Steam/.../Portal 2/portal2_arm64_test` no existe en este entorno; ejecuté la sonda en un stage aislado de `/private/var/.../T/opencode/portal2-paint-probe-stage`, montando el contenido instalado mediante symlinks. En `diagnostics/20261027-001608-540206/engine.log`, `scripts/test-portal2-render.py --map sp_a3_portal_intro --paint-probe` registró `SV_ActivateServer`, 22 aspersores configurados, 22 streams, un cleanser, `paintmap=0` y 609 blobs. También registró `Think`, `Start`/`Stop` y el no-op de `PaintBlobUpdate`. `PAINT PROBE PASS` valida el arranque/configuración, no movimiento ni pintura; persistieron errores de shader y recursos.

La ruta de movimiento existente en `paint_blobs_shared.cpp` se intentó integrar, pero no compila con esta rama: faltan `IsPointInBounds`, `CTraceFilterNoPlayers` y APIs/campos de `CTrigger_TractorBeam`. Se retiró esa inclusión para conservar un build válido; no se copió ni sustituyó el pseudocódigo descompilado. Hubo un relanzamiento intermedio que terminó por una excepción de AppKit (`NSInternalInconsistencyException`); la ejecución posterior sí llegó a la sonda, sin cambiar el límite de shader ni demostrar gameplay.

**Pendiente:** reparar las dependencias/compatibilidad que impiden compilar el actualizador de blobs y reconstruir de forma independiente la capa `paintmap` del motor. La ausencia de marcadores de colisión/aplicación se explica por el stub anterior; no se afirma compatibilidad de pintura en mundo, persistencia, cleanser sobre pintura ya aplicada, velocidad ni rebote.

---

# 65. Capítulo inicial y fallback de texturas

- `materialsystem/texturemanager.cpp` ahora genera una textura de error gris uniforme (`RGB 112`) en lugar del patrón magenta/negro. `IsErrorTexture` sigue activo y no se silencian los avisos de recursos faltantes. `ctexture.cpp` conserva la ruta que usa ese fallback cuando falla una carga.
- `python3 waf build` terminó correctamente tras el cambio. En una captura de `sp_a1_intro1` desde el stage aislado (`diagnostics/20261027-003137-666634/map.png`) no aparece el patrón magenta/negro; sí hay objetos cian por combos de shader no encontrados. La captura no fuerza aisladamente un VTF faltante, así que valida el estado visible de ese mapa, no todas las rutas de fallback.
- La carga del mapa activó el servidor, pero `test-portal2-render.py` informó `MAP LOAD FAIL`: `lightmappedgeneric_ps20b.vcs`, `skin_ps20b.vcs` y `vertexlit_and_unlit_generic_ps20b.vcs` no contienen combos solicitados. También faltan proxies `FizzlerVortex`, `LightedFloorButton` y `LightedMouth`; no se deben atribuir esos defectos al fallback de textura.
- La fixture `--elevator-transition` completó `sp_a1_intro1 -> sp_a1_intro2`, `PORTAL2_ARRIVAL ride=PASS` y `exit=PASS`. Su primera ejecución quedó como `FAIL` por el shader global; el test ahora mantiene `shader_index_error=True` en el reporte, pero permite que la aserción independiente de transición dé `PASS`.
- En `sp_a1_intro2`, la fixture `--buttons` dio `BUTTONS PASS` para seleccionar y reutilizar los cuatro pedestales/portales; `shader_index_error=True` sigue siendo un fallo visual independiente.
- `--intro2-exit` con `sp_a1_intro2` pasó `sp_a1_intro2 -> sp_a1_intro3` y el viaje/salida de ambos ascensores. La fixture activa por sí misma `portal2_ground_probe`; la ejecución previa de 60 s no alcanzó las aserciones finales.
- Los lumps de entidades de `sp_a1_intro5` y `sp_a1_intro6` confirman el mismo rig y callbacks de salida; añadí `intro5 -> intro6`, `intro6 -> intro7` y la llegada en `intro6`/`intro7`. La fixture desde `sp_a1_intro6` pasó llegada, salida, cambio a `sp_a1_intro7` y llegada/salida allí.
- Una ejecución desde `sp_a1_intro5` cayó una vez con `SIGSEGV` en `C_BaseEntity::PostDataUpdate` durante la llegada a intro6. Dos repeticiones posteriores recorrieron `intro5 -> intro6 -> intro7`; la repetición limpia de 100 s terminó `INTRO EXIT PASS` con el proceso activo. El crash no se reprodujo, pero queda registrado como fallo transitorio observado.
- Añadí las rutas de capítulo `sp_a1_wakeup -> sp_a1_intro1` y `sp_a1_intro7 -> sp_a2_intro`. Ambas fixtures dieron `CHAPTER TRANSITION PASS`; `shader_index_error=True` se conserva en el reporte como fallo visual independiente.
- El stage temporal no contiene `scripts/portal2_intro1_scenes.txt` ni una imagen de escenas compatible; el log reporta `incompatible scene image`, escena faltante y `PORTAL2_INTRO missing scene data`. La prueba de arranque no valida la secuencia cinemática. El stage de Steam solicitado no existe en esta máquina; se usó `/private/var/.../T/opencode/portal2-paint-probe-stage`.
- Las transiciones de ascensor implementadas abarcan `sp_a1_intro2`–`sp_a1_intro6` y las llegadas hasta `sp_a1_intro7`. El paso completo por ascensores ha pasado fixtures; se observó un `SIGSEGV` transitorio en una ejecución de `intro5 -> intro6`, no reproducido en las dos repeticiones posteriores.

**Pendiente:** preparar escenas y VCS/proxies correctos para el stage ARM64 y jugar el capítulo con captura interactiva. Hasta entonces no se afirma “capítulo jugable sin problemas”.

---

# 66. Grounding del jugador en bordes de escalón

- `CPortalGameMovement::CategorizePosition` intentaba recuperar el suelo con cuatro sub-traces, pero después clasificaba el resultado usando `traceNormalAngle` del trace central anterior. Si el sub-trace encontraba una cara transitable, esa normal nueva se descartaba y `GroundEntity` podía quedar en `NULL`; `WalkMove` entonces no intenta subir escalones.
- Ahora se calcula la normal después del sub-trace y se usa para aceptar/rechazar el suelo. Se recompilaron los objetos compartidos del cliente y del servidor y ambos módulos enlazaron correctamente.
- Intenté automatizar una subida en `sp_a1_wakeup`, pero el punto de depuración está en la zona de la escalera destruida (`glados_staircase_destruction`/`stair_fall_clip`); la fixture no reproduce una escalera transitable y la retiré en vez de contar su `FAIL` como resultado del fix.

**Pendiente:** validar en el mapa y la escalera concreta que viste, con movimiento jugable y captura. El arreglo corrige una pérdida real del suelo, pero no prueba por sí solo que elimine todos los casos de flotamiento o bloqueo.

---

# 67. Referencias EpicSentry para Paint

- Descargué `EpicSentry/P2ASW` en `work/ref-P2ASW` (commit `0df06fb`) y `EpicSentry/Portal-2` en `work/ref-Portal-2` (commit `72e7358`). Son fuentes de consulta separadas; no se incorporan como submódulos ni se copian árboles completos.
- La pieza pequeña y directamente necesaria fue el orden de `PaintPowerType` de P2ASW: bounce `0`, reflect `1`, speed `2`, portal `3` y sin pintura/limpiador `4`. El enum local ahora respeta los valores serializados por los mapas; el probe exige `expected=2 observed=2`. Los módulos grandes de `CPaintDatabase`, guardado y `Blobulator` dependen de APIs, mensajes y entidades ausentes o distintas en este fork, por lo que no se trasplantaron a ciegas.
- `Portal-2` es una base ASW de 2016. Su `portal_gamemovement.cpp` sirve como contraste de movimiento de Portal, pero no aporta una implementación de Paint más completa que P2ASW para el port actual.
- El README de P2ASW dice que macOS/Linux no están probados oficialmente y restringe el uso a no comercial con atribución; preservar esos avisos y revisar las licencias antes de distribuir cambios derivados.

## Prueba tras comparar las referencias

`python3 scripts/test-portal2-render.py ... --map sp_a3_portal_intro --paint-probe --screenshot --seconds 35` terminó `PAINT PROBE PASS`: servidor activo, 22/22 aspersores configurados, 22 streams, un cleanser, 78 blobs y `paintmap=1`. El log registra `PORTAL2_PAINT_APPLY target=worldspawn type=2 result=world-painted` y `PORTAL2_PAINT_ROUNDTRIP expected=2 observed=2 PASS` en `diagnostics/20261027-015448-300127/engine.log`.

La captura `diagnostics/20261027-015448-300127/map.png` salió gris uniforme. Por tanto, la prueba demuestra depósito y consulta del color en el servidor, **no** el render de la pintura. Persisten shaders/recursos faltantes en el stage; `shader_index_error=True`. Siguiente pieza necesaria para que el mapa muestre Paint: un backend de render del paintmap que funcione con el renderer y shaders ARM64 del port, y después replicación/persistencia cliente.

---

# 68. Estabilización del capítulo 1 en ARM64

- `CLocalNetworkBackdoor` ahora conserva el serial asociado a cada actualización creada/cambiada y verifica que índice, serial y `IClientNetworkable` sigan siendo los actuales antes de `PostDataUpdate`. También reconcilia la caché antes de las rutas de entidad dormida y revisa la entidad después de `PostDataUpdate` antes de notificar transmisión. Las entradas obsoletas se descartan sin despachar una vtable reciclada.
- Al adoptar una entidad que ya estaba creada con el serial actual, la ruta podía encolarse dos veces como `DATA_UPDATE_CREATED` si estaba dormida. Evité la segunda encolada al reactivarla; así queda una sola notificación de creación por ciclo.
- `python3 waf build --target=engine,client,server` terminó correctamente; `libengine.dylib`, `libclient.dylib` y `libserver.dylib` son Mach-O ARM64.
- En el stage aislado `/private/var/folders/76/nz4yws2s6wd31b05l0rq9v080000gn/T/opencode/portal2-paint-probe-stage` activé los overlays que faltaban en `gameinfo.txt`. Generé con herramientas existentes el script de 49 entradas de escenas, la imagen compatible con 4663 registros, 63 audios, 580 archivos del renderer compatible y 59 adaptaciones de materiales. Los originales permanecen montados como solo lectura; las adaptaciones están fuera del repositorio.
- `scripts/test-portal2-render.py` conserva fixtures acotadas por `--seconds`; no termina anticipadamente al ver sus marcadores.
- Tres pasadas previas con el build ARM64 recorrieron `sp_a1_intro5 -> sp_a1_intro6 -> sp_a1_intro7`; las llegadas y salidas de ascensor dieron `PASS`, el servidor siguió activo y no se observó otro `SIGSEGV`. Eran ejecuciones anteriores al último cambio de reconciliación de seriales.
- Con la reconciliación y la cola corregidas, la fixture desde `sp_a1_intro4` llegó a cargar `sp_a1_intro5` y activar el servidor sin crash, pero terminó `INTRO EXIT FAIL`: no arrancó la llegada del ascensor en intro5. El log no contiene `PORTAL2_RIG` ni `PORTAL2_ARRIVAL started` para intro5. `Missing native function: OnPostTransition` también aparece en intro4, donde el fallback sí inicia el ascensor, así que ese aviso por sí solo no explica la diferencia. El tramo `intro5 -> intro6` sigue bloqueado y su causa inmediata queda sin aislar.
- En una ejecución previa se observó `SIGTRAP` al asignar `C_InfoPlacementHelper`; el informe de macOS indica corrupción de un bloque del heap. La corrección de doble encolado elimina un defecto real en la secuencia de notificaciones, pero no se ha demostrado que explique o resuelva por sí sola esa corrupción.
- El montaje resuelve las escenas y reduce los fallos de recursos, pero quedan combinaciones ausentes de `skin_ps20b` y `vertexlit_and_unlit_generic_ps20b`. No doy por verificada la visibilidad del jugador/modelos ni una pasada interactiva completa hasta inspeccionar capturas y terminar las pruebas de apertura, botones y escenas.
- Comparé la rutina simple de referencia Portal 1 con la implementación estilo Portal 2 de este árbol: la primera usa gravedad vertical y trazas básicas; la segunda añade manejo de plataformas móviles y otras reglas. No se reemplazó el archivo completo porque perdería soporte que las pruebas de ascensores necesitan.

**Pendiente:** aislar por qué intro5 no inicia los probes/adaptadores de llegada después de activar el servidor; aislar la corrupción de memoria detectada al crear `C_InfoPlacementHelper`; repetir tres veces `intro5 -> intro6`; completar las fixtures de escenas, botones y transiciones inicial/final con el build final; y comprobar visualmente los mapas. Persisten warnings de shader de modelos y no hay una pasada interactiva completa. El capítulo aún no cumple el criterio de aceptación de “sin crashes ni bloqueos”.
