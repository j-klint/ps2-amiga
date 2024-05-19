# ps2-amiga

Tällä saa PS/2-hiiren toimimaan Amiga 500:ssa Arduino Unon välityksellä.
Oikestaan ei tämä Arduinosta taida muuta käyttää, kuin itse ATmega328-prosessoria ja `<Arduino.h>`:sta jotain `millis()`-funktiota timeouttien kyyläämiseen. Interrupteilla se biti hoitelee piuhoihin.

Ennen kuin uskaltauduin tämmöistä kytkemään kallisarvoiseen Amigaan kiinni, niin testailin PS/2-puolta kirjoittelemalla semmoisen, että voi siinä Elegoon starter kitin mukana tulleen LCD-näytön näytöllä hiirellä heilutella tähtäintä. Se on nyt pois käytöstä. Jos haluatte sillä leikkiä, niin kommentoikaa `platformio.ini`:stä nuo `lib_deps`-rivit käyntiin.

Yritin upata juutuubiin pienen esittelyvideon: <https://www.youtube.com/watch?v=8wuTVJKSxSc>

## Ohjeita

Ohjelmia pitäis olla:

- VSCode ja siihen lisäosat
    - C/C++
    - PlatformIO IDE

Tuo PlatformIO näyttää olevan niin fiksu, että se itsestään hoksaa, mihin "sarjaporttiin" on Arduinon johto tökkäisty. Ei tarvi paljon muuta kuin build- ja upload-nappeja painella.

Saattaa tämä Arduino IDE:lläkin toimia, jos vaihtaa tuon `main.cpp`:n nimen. En ole kokeillut.

Kytkentäkaaviota en tähän hätään saanut piirretyksi, mutta periaatteessa johdinten paikat pitäisi voida päätellä lähdekoodista, jos ymmärtää Atmegan porttien rekisterien päälle.

Vehje saa sitten 5 voltin käyttöjännitteensä Amigan hiiriportista, joten ei liene suotavaa, että se olisi USB-piuhalla samaan aikaan kytkettynä toiseenkin tietokoneeseen!
