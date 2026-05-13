# STM32WBA Zigbee IR Sensor Project

Tento projekt implementuje Zigbee End Device (Router) na platformě STM32WBA (Nucleo-WBA52CG), 
který kombinuje funkci dálkového ovládání světel a automatickou detekci překážek 
pomocí infračerveného (IR) senzoru.

--- HLAVNÍ FUNKCE ---
- Zigbee 3.0 Connectivity: Automatické připojení k síti na pevném kanálu 14.
- IR Senzor (PB2): Monitorování stavu v reálném čase. Při detekci překážky 
  (LOW na pinu PB2) se rozsvítí červená LED a odešle se stav do Zigbee sítě.
- Occupancy Sensing: Stav senzoru je reportován skrze standardní Zigbee cluster 
  'Occupancy Sensing' (ZCL).
- Dálkové ovládání (B1): Fyzické tlačítko na desce posílá příkaz 'Toggle' 
  do koordinátoru (např. pro zapnutí/vypnutí světla).
- Debug konzole: Podrobný log událostí přes UART (115200 baud).

--- HARDWAROVÉ ZAPOJENÍ ---
- IR Senzor      -> Pin PB2 (Vstup, Pull-up)
- User Button 1 -> Pin B1  (Toggle On/Off příkaz)
- Status LED     -> Red LED (Indikace detekce překážky)
- Debug UART    -> LPUART1 (Výpis stavových zpráv)

--- STRUKTURA KLÍČOVÝCH SOUBORŮ ---
- main.c: Hlavní smyčka, čtení IR senzoru a ošetření logiky změn stavu.
- app_zigbee.c: Inicializace Zigbee stacku a správa síťového spojení.
- app_zigbee_endpoint.c: Definice koncových bodů (Endpoints) a clusterů 
  (OnOff Client, Occupancy Server).

--- ZIGBEE KONFIGURACE ---
- Role: Router / End Device
- Kanál: 14
- Cluster ID: 
    - 0x0006 (On/Off Client)
    - 0x0406 (Occupancy Sensing Server)
