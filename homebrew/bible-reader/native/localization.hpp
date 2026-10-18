// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Book order is canonical and shared with the offline verse indices.
#include <array>
#include <string>
#include <vector>
namespace bible_i18n {
enum class Key:int {
 library, oldTestament, newTestament, bookmarks, settings, bible,
 chooseBook, chooseChapter, searchBible, searchHint, searchResults,
 savedVerses, matches, noMatches, noBookmarks, fontSize, dailyPanel,
 colourTheme, language, returnReading, midnight, slate, on, off,
 changeChapter, searchHelp, dailyVerse, offlineMissing, reinstall,
 exitQuestion, exitAnswer, offlineStatus, reading, booksTitle, version, controls, ttsStop, voiceStyle, neuralPreview, classicVoice, about
};
constexpr int kLangCount=4;
constexpr int kLanguageOptions=6; // old 0-3, retro 4, Russian 5
inline int language(int n){return n>=0&&n<kLangCount?n:0;}
inline const char* code(int n){
 static constexpr const char* codes[]={"en","de","fr","es"};
 return n==5?"ru":codes[language(n)];
}
inline const char* languageName(int n){
 static constexpr const char* names[]={"English","Deutsch","Français","Español"};
 return n==4?"Stephen Hawking (Retro TTS)":n==5?"Русский":names[language(n)];
}
inline const char* translationName(int n){
 static constexpr const char* names[]={
  "World English Bible (WEBPB)", "Lutherbibel 1912",
  "Louis Segond 1910", "Reina-Valera 1909"
 };
 return n==5?"Синодальный перевод 1876":names[language(n)];
}
inline const char* label(int lang,Key key){
 if(lang==5){
  switch(key){
   case Key::library:return "Библиотека";
   case Key::oldTestament:return "Ветхий Завет";
   case Key::newTestament:return "Новый Завет";
   case Key::bookmarks:return "Закладки";
   case Key::settings:return "Настройки";
   case Key::bible:return "БИБЛИЯ";
   case Key::chooseBook:return "Выберите книгу";
   case Key::chooseChapter:return "Выберите главу";
   case Key::searchBible:return "ПОИСК ПО БИБЛИИ";
   case Key::searchHint:return "Введите слово или фразу";
   case Key::searchResults:return "РЕЗУЛЬТАТЫ ПОИСКА";
   case Key::savedVerses:return "сохранённые стихи";
   case Key::matches:return "совпадения (до 150)";
   case Key::noMatches:return "Совпадений нет.";
   case Key::noBookmarks:return "Закладок нет.";
   case Key::fontSize:return "Размер шрифта";
   case Key::dailyPanel:return "Стих дня";
   case Key::colourTheme:return "Цветовая тема";
   case Key::language:return "Язык";
   case Key::returnReading:return "Вернуться к чтению";
   case Key::midnight:return "Ночь";
   case Key::slate:return "Тёмно-синий";
   case Key::on:return "Вкл.";
   case Key::off:return "Выкл.";
   case Key::changeChapter:return "L1 / R1  Сменить главу";
   case Key::searchHelp:return "R2 Поиск   КВАДРАТ Удалить   O Назад";
   case Key::dailyVerse:return "С Т И Х   Д Н Я";
   case Key::offlineMissing:return "Библия недоступна";
   case Key::reinstall:return "Переустановите архив приложения";
   case Key::exitQuestion:return "ВЫЙТИ ИЗ BIBLE READER?";
   case Key::exitAnswer:return "X Да, выйти     O Нет, продолжить";
   case Key::offlineStatus:return "100% офлайн - ";
   case Key::reading:return "Чтение ";
   case Key::booksTitle:return "КНИГИ";
   case Key::version:return "Версия";
   case Key::controls:return "X Выбор   O Назад   КВАДРАТ Закладка   L1/R1 Главы";
   case Key::ttsStop:return "ЧИТАЕТ   O СТОП";
   case Key::voiceStyle:return "Тип голоса";
   case Key::neuralPreview:return "Neural HD (5 языков)";
   case Key::classicVoice:return "Классика";
   case Key::about:return "О программе";
  }
 }
 if(key==Key::about){
  static constexpr const char* labels[]={"About","Über","À propos","Acerca de"};
  return labels[language(lang)];
 }
 static constexpr const char* entries[][kLangCount]={
  {"Library","Bibliothek","Bibliothèque","Biblioteca"},
  {"Old Testament","Altes Testament","Ancien Testament","Antiguo Testamento"},
  {"New Testament","Neues Testament","Nouveau Testament","Nuevo Testamento"},
  {"Bookmarks","Lesezeichen","Favoris","Marcadores"},
  {"Settings","Einstellungen","Paramètres","Ajustes"},
  {"BIBLE","BIBEL","BIBLE","BIBLIA"},
  {"Choose a book","Buch wählen","Choisir un livre","Elegir un libro"},
  {"Choose a chapter","Kapitel wählen","Choisir un chapitre","Elegir capítulo"},
  {"SEARCH THE BIBLE","BIBEL DURCHSUCHEN","RECHERCHER DANS LA BIBLE","BUSCAR EN LA BIBLIA"},
  {"Enter a word or phrase","Wort oder Satz eingeben","Entrer un mot ou une phrase","Escribe una palabra o frase"},
  {"SEARCH RESULTS","SUCHERGEBNISSE","RÉSULTATS DE RECHERCHE","RESULTADOS DE BÚSQUEDA"},
  {"saved verses","gespeicherte Verse","versets enregistrés","versículos guardados"},
  {"matches (up to 150)","Treffer (max. 150)","résultats (max. 150)","resultados (máx. 150)"},
  {"No matching verses.","Keine passenden Verse.","Aucun verset trouvé.","No se encontraron versículos."},
  {"No bookmarks yet.","Noch keine Lesezeichen.","Aucun favori.","Aún no hay marcadores."},
  {"Font size","Schriftgröße","Taille du texte","Tamaño de letra"},
  {"Daily verse panel","Tagesvers anzeigen","Verset du jour","Versículo diario"},
  {"Colour theme","Farbschema","Thème de couleur","Tema de color"},
  {"Language","Sprache","Langue","Idioma"},
  {"Return to reading","Zurück zum Lesen","Retour à la lecture","Volver a leer"},
  {"Midnight","Mitternacht","Minuit","Medianoche"},
  {"Slate","Schiefer","Ardoise","Pizarra"},
  {"On","Ein","Activé","Sí"},
  {"Off","Aus","Désactivé","No"},
  {"L1 / R1  Change chapter","L1 / R1  Kapitel wechseln","L1 / R1  Changer de chapitre","L1 / R1  Cambiar capítulo"},
  {"R2 Search   SQUARE Delete   O Back","R2 Suchen   SQUARE Löschen   O Zurück","R2 Rechercher   SQUARE Effacer   O Retour","R2 Buscar   SQUARE Borrar   O Atrás"},
  {"D A I L Y   V E R S E","T A G E S V E R S","V E R S E T   D U   J O U R","V E R S Í C U L O   D E L   D Í A"},
  {"Offline Bible unavailable","Offline-Bibel nicht verfügbar","Bible hors ligne indisponible","Biblia sin conexión no disponible"},
  {"Reinstall the complete Bible Reader ZIP","Installations-ZIP erneut installieren","Réinstallez le ZIP complet","Reinstala el ZIP completo"},
  {"EXIT BIBLE READER?","BIBLE READER BEENDEN?","QUITTER BIBLE READER ?","¿SALIR DE BIBLE READER?"},
  {"X Yes, close app     O No, keep reading","X Ja, schließen     O Nein, weiterlesen","X Oui, fermer     O Non, continuer","X Sí, cerrar     O No, continuar"},
  {"100% offline - ","100 % offline - ","100 % hors ligne - ","100 % sin conexión - "},
  {"Reading ","Lese ","Lecture ","Leyendo "},
  {"BOOKS","BÜCHER","LIVRES","LIBROS"},
  {"Version","Version","Version","Versión"},
  {"X Select   O Back   SQUARE Bookmark   TRIANGLE Search   L1/R1 Chapters   OPTIONS Settings   L2 Daily verse",
   "X Wählen   O Zurück   SQUARE Merken   TRIANGLE Suche   L1/R1 Kapitel   OPTIONS Einstellungen   L2 Tagesvers",
   "X Choisir   O Retour   SQUARE Favoris   TRIANGLE Recherche   L1/R1 Chapitre   OPTIONS Paramètres   L2 Verset du jour",
   "X Elegir   O Atrás   SQUARE Marcador   TRIANGLE Buscar   L1/R1 Capítulo   OPTIONS Ajustes   L2 Versículo diario"},
  {"READING   O STOP","VORLESEN   O STOP","LECTURE   O ARRÊTER","LEYENDO   O PARAR"},
  {"Voice style","Stimmqualität","Style de voix","Estilo de voz"},
  {"Neural HD (5 languages)","Neural HD (5 Sprachen)","Neural HD (5 langues)","Neural HD (5 idiomas)"},
  {"Classic","Klassisch","Classique","Clásica"}
 };
 const int i=(int)key;
 return i>=0&&i<(int)(sizeof(entries)/sizeof(entries[0]))?entries[i][language(lang)]:"";
}
inline const char* bookName(int lang,int index){
 static constexpr const char* de[]={
 "1. Mose","2. Mose","3. Mose","4. Mose","5. Mose","Josua","Richter","Ruth",
 "1. Samuel","2. Samuel","1. Könige","2. Könige","1. Chronik","2. Chronik",
 "Esra","Nehemia","Ester","Hiob","Psalmen","Sprüche","Prediger","Hohelied",
 "Jesaja","Jeremia","Klagelieder","Hesekiel","Daniel","Hosea","Joel","Amos",
 "Obadja","Jona","Micha","Nahum","Habakuk","Zefanja","Haggai","Sacharja",
 "Maleachi","Matthäus","Markus","Lukas","Johannes","Apostelgeschichte",
 "Römer","1. Korinther","2. Korinther","Galater","Epheser","Philipper",
 "Kolosser","1. Thessalonicher","2. Thessalonicher","1. Timotheus","2. Timotheus",
 "Titus","Philemon","Hebräer","Jakobus","1. Petrus","2. Petrus","1. Johannes",
 "2. Johannes","3. Johannes","Judas","Offenbarung"
 };
 static constexpr const char* fr[]={
 "Genèse","Exode","Lévitique","Nombres","Deutéronome","Josué","Juges","Ruth",
 "1 Samuel","2 Samuel","1 Rois","2 Rois","1 Chroniques","2 Chroniques",
 "Esdras","Néhémie","Esther","Job","Psaumes","Proverbes","Ecclésiaste",
 "Cantique des cantiques","Ésaïe","Jérémie","Lamentations","Ézéchiel","Daniel",
 "Osée","Joël","Amos","Abdias","Jonas","Michée","Nahum","Habacuc","Sophonie",
 "Aggée","Zacharie","Malachie","Matthieu","Marc","Luc","Jean","Actes",
 "Romains","1 Corinthiens","2 Corinthiens","Galates","Éphésiens",
 "Philippiens","Colossiens","1 Thessaloniciens","2 Thessaloniciens",
 "1 Timothée","2 Timothée","Tite","Philémon","Hébreux","Jacques",
 "1 Pierre","2 Pierre","1 Jean","2 Jean","3 Jean","Jude","Apocalypse"
 };
 static constexpr const char* es[]={
 "Génesis","Éxodo","Levítico","Números","Deuteronomio","Josué","Jueces","Rut",
 "1 Samuel","2 Samuel","1 Reyes","2 Reyes","1 Crónicas","2 Crónicas",
 "Esdras","Nehemías","Ester","Job","Salmos","Proverbios","Eclesiastés",
 "Cantares","Isaías","Jeremías","Lamentaciones","Ezequiel","Daniel",
 "Oseas","Joel","Amós","Abdías","Jonás","Miqueas","Nahúm","Habacuc",
 "Sofonías","Hageo","Zacarías","Malaquías","Mateo","Marcos","Lucas",
 "Juan","Hechos","Romanos","1 Corintios","2 Corintios","Gálatas",
 "Efesios","Filipenses","Colosenses","1 Tesalonicenses","2 Tesalonicenses",
 "1 Timoteo","2 Timoteo","Tito","Filemón","Hebreos","Santiago",
 "1 Pedro","2 Pedro","1 Juan","2 Juan","3 Juan","Judas","Apocalipsis"
 };
 static constexpr const char* ru[]={
  "Бытие",
  "Исход",
  "Левит",
  "Числа",
  "Второзаконие",
  "Иисус Навин",
  "Судьи",
  "Руфь",
  "1 Царств",
  "2 Царств",
  "3 Царств",
  "4 Царств",
  "1 Паралипоменон",
  "2 Паралипоменон",
  "Ездра",
  "Неемия",
  "Есфирь",
  "Иов",
  "Псалтирь",
  "Притчи",
  "Екклесиаст",
  "Песнь Песней",
  "Исаия",
  "Иеремия",
  "Плач Иеремии",
  "Иезекииль",
  "Даниил",
  "Осия",
  "Иоиль",
  "Амос",
  "Авдий",
  "Иона",
  "Михей",
  "Наум",
  "Аввакум",
  "Софония",
  "Аггей",
  "Захария",
  "Малахия",
  "Матфей",
  "Марк",
  "Лука",
  "Иоанн",
  "Деяния",
  "Римлянам",
  "1 Коринфянам",
  "2 Коринфянам",
  "Галатам",
  "Ефесянам",
  "Филиппийцам",
  "Колоссянам",
  "1 Фессалоникийцам",
  "2 Фессалоникийцам",
  "1 Тимофею",
  "2 Тимофею",
  "Титу",
  "Филимону",
  "Евреям",
  "Иаков",
  "1 Петра",
  "2 Петра",
  "1 Иоанна",
  "2 Иоанна",
  "3 Иоанна",
  "Иуда",
  "Откровение"
 };
 if(index<0||index>=66)return "";
 if(lang==5)return ru[index];
 if(lang==1)return de[index];
 if(lang==2)return fr[index];
 if(lang==3)return es[index];
 return "";
}
inline std::vector<std::string> searchKeys(int lang){
 std::vector<std::string> v;
 const std::string latin="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -'<";
 for(char c:latin)v.emplace_back(1,c);
 const char* accent=lang==5?"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя":lang==1?"ÄÖÜäöüß":lang==2?"ÉÈÊËÀÂÔÎÙÛÇŒéèêëàâôîùûçœ":lang==3?"ÁÉÍÓÚÜÑáéíóúüñ¿¡":"";
 // UTF-8 split (never cut a multibyte input glyph).
 for(size_t i=0;accent[i];){
  unsigned char b=(unsigned char)accent[i];size_t n=b<0x80?1:b<0xe0?2:b<0xf0?3:4;
  v.emplace_back(accent+i,n);i+=n;
 }
 return v;
}
inline void popUtf8(std::string& input){
 if(input.empty())return;
 // First strip UTF-8 continuation bytes; then remove the leading codepoint byte.
 while(!input.empty()&&(((unsigned char)input.back()&0xC0)==0x80))input.pop_back();
 if(!input.empty())input.pop_back();
}
} // namespace bible_i18n
