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
