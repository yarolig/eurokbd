<table>
<tr><td width='50%' valign='top'>
Написал я тут свою клаву для КПК, хотя их есть много и разных, но...<br />
</td><td width='50%' valign='top'>
Despite the fact that there exist many good keyboards for Pocket PC, I decided to create my own...<br>
</td></tr>

<tr><td width='50%' valign='top'>
Во-первых, мне реально нужна мультиязычность, заметно большая, чем даёт правый <b>Alt</b> в раскладке "US International" - там например нет чешских букв (вроде <b>Ř</b> и <b>Ů</b>), а чешско-русским словарём в lingvo приходится пользоваться часто. <br />
</td><td width='50%' valign='top'>
First of all, I need support for many languages, more extensive than just using <b>Right-Alt</b> in the "US International" layout (on a PC or on PC emulation keyboards like vgakey.net), for instance it does not have any Czech letters (like <b>Ř</b> or <b>Ů</b>), and as I use Czech-Russian dictionary (lingvo.ru) very often and I need to type both Czech and Russian characters.<br>
</td></tr>

<tr><td width='50%' valign='top'>
И не тока чешским - в итоге мой набор необходимых символов получается большим и странным.<br />
</td><td width='50%' valign='top'>
Sometimes I use some other languages with their typical characters (Spanish, ...). Thus, the character set I need to operate with is big and specific.<br>
</td></tr>

<tr><td width='50%' valign='top'>
Вот этот пункт был очень важный, искал готовое решение, но не нашёл.<br>
</td><td width='50%' valign='top'>
As this is of great importance for me, I had been looking for a ready-made solution but without any success.<br>
</td></tr>

<tr><td width='50%' valign='top'>
Зато сейчас сделал с запасом - не переключая раскладку можно ввести любую букву любого европейского языка :)<br />
</td><td width='50%' valign='top'>
The present solution actually exceeds my needs - it is possible to enter any character of any European language without endless switching of layouts.<br>
</td></tr>

<tr><td width='50%' valign='top'>
Во-вторых, перенос идеи механической клавиатуры на тачскрин мне всегда не нравился: какие-то возможности пропали (ну например нажать и держать <b>Shift</b>+стрелку наслаждаясь автоповтором), а взамен ничего не появилось.<br />
</td><td width='50%' valign='top'>
Secondly, I have never been happy with the idea of one-to-one translation of physical keyboard behavior on the touch-screen: some features disappear (for example, automatic repeating while holding <b>Shift</b>+arrow), but not a single advantage appears in return.<br>
</td></tr>

<tr><td width='50%' valign='top'>
А, еще <b>Shift</b> и <b>CapsLock</b> на тачскрине выглядят как-то нелепо.<br />
</td><td width='50%' valign='top'>
Also, <b>Shift</b> and <b>CapsLock</b> look out of place on a touch screen.<br>
</td></tr>

<tr><td width='50%' valign='top'>
В итоге решил развить идею, которая есть на многих клавиатурах - "смещение стилуса вверх -> клавиша с шифтом". идея очень хорошая, только если эту букву на экране нарисовать, будут видны границы этого "вверха".<br>
</td><td width='50%' valign='top'>
Finally, I decided to develop the idea, which is implemented in many virtual keyboards: "drag slylus up to get upcased letter". This idea is very elegant, but I find one addition necessary: we should prompt user with region borders where to release stylus in case he wants to type the uppercase letter, or lowercase letter, or to type nothing.<br>
</td></tr>

<tr><td width='50%' valign='top'>
Тогда можно несколько кнопочек там разместить. При смещении курсора после нажатия на <b>u</b> получатся не только <b>U</b> но заодно и <b>Ů</b> и <b>Г</b>. И <b>Ctrl-U</b>.<br /><br />
А на кнопках <b>1-10</b> можно разместить <b>F1-F12</b><br /><br />
Как-то так:<br>
</td><td width='50%' valign='top'>
This concept makes it possible to add more than one additional letter. The button <b>u</b> can help us type not only <b>u</b> and <b>U</b>, but also <b>Ů</b> and <b>Ü</b>. As well as <b>Ctrl-U</b>.<br /><br />
Keys <b>1-0</b> can serve as <b>F1-F12</b>.<br /><br />
We can look at the picture for better understanding:<br>
</td></tr>

<tr><td align='center'>
<img width='480' height='480' src='http://eurokbd.googlecode.com/files/screenshot1.jpg' /><br />
</td></tr>

<tr><td align='center'>
<img width='480' height='300' src='http://eurokbd.googlecode.com/files/screenshot2.jpg' /><br />
</td></tr>

<tr><td width='50%' valign='top'>
Вроде нагромождение жуткое, но:<br />
1. На картинках всё по максимуму, что не нужно - закоменчивается в конфиге, это проще чем добавлять :)<br>
</td><td width='50%' valign='top'>
Looks a bit cluttered, but this layout has some advantages:<br />
1. At the pictures almost all possible keys are visible. If you do not need some of them (actually, you won’t need more than half of them), you can comment them out. It is more simple than adding new keys, that’s why so many characters are enabled by default.<br>
</td></tr>

<tr><td width='50%' valign='top'>
2. Становятся легко доступны всякие значки вроде <b>± † ™ ∞ ≈ ¿ ¾</b> и буквы с ударениями<br>
</td><td width='50%' valign='top'>
2. Easy and intuitive access to symbols (like <b>± † ™ ∞ ≈ ¿ ¾</b>) and accented characters of national alphabets.<br>
</td></tr>

<tr><td width='50%' valign='top'>
3. Всякие комбинации screen типа<b>Ctrl-A</b> <b>Shift-S</b> набираются в 2 тыка (<b>A</b>+вниз <b>S</b>+вверх), а не в 4.<br>
</td><td width='50%' valign='top'>
3. Ugly key combinations of screen (unix session manager, if you using Pocket PC as ssh client for your unix computers) like <b>Ctrl-A Shift-S</b> can be entered in two touches (<b>A</b>-dragdown, <b>S</b>-dragup) instead of four.<br>
</td></tr>

<tr><td width='50%' valign='top'>
4. При наборе русской буквы основная раскладка переключатеся на русскую без дополнительной кнопки <b>Rus/Lat</b> и наоборот при наборе латинской буквы при основной русской)<br>
</td><td width='50%' valign='top'>
4. Entering a Russian letter switches main layout to Russian automatically, with no touch of additional <b>Rus/Lat</b> key which serve only for change layouts. Changing from Russian to Latin also works, of course.<br>
</td></tr>

<tr><td width='50%' valign='top'>
5. Автоповтор работает на любой комбинации с <b>Shift</b> и <b>Ctrl</b>.<br>
</td><td width='50%' valign='top'>
5. Automatic repeat works for any combination involving <b>Shift</b> or <b>Ctrl</b> keys.<br>
</td></tr>

<tr><td width='50%' valign='top'>
6. Можно выкинуть горизонтальный ряд <b>F1-F10</b> и вертикальный <b>Tab-CapsLock-Shift-Ctrl</b> и сделать кнопки крупнее, чтобы было удобно на них нажимать пальцем без стилуса.<br>
</td><td width='50%' valign='top'>
6. It is possible to remove the row of keys <b>F1-F10</b> and the column <b>Tab-CapsLock-Shift-Ctrl</b> in order to make the rest of the keys a little bigger, thus allowing to press them by fingers.<br>
</td></tr>

<tr><td width='50%' valign='top'>
И этому привыкаешь быстро, так же как и сдвигать стилус вверх для ввода большой буквы, глазами искать приходится только если реально редко используемая буква нужна.<br>
</td><td width='50%' valign='top'>
It’s very fast and easy to get used to the new way of typing, as to hold'n'drag using any other virtual keyboard. You will need to find only rarely used characters, because it is very easy to remember where the frequently used characters are located. At least, you can always relocate (or duplicate) them by changing the configuration file.<br>
</td></tr>

<tr><td width='50%' valign='top'>
Попап всплывает или через 300ms удержания стилуса на кнопке или при уводе стилуса за её границы. То есть оно не мешает набирать текст быстро тыкая.<br>
</td><td width='50%' valign='top'>
The popup window appears after 300ms from holding stylus over a button or immediately on dragging stylus out of the bounds of the button. So, the popup does not interfere with fast typing.<br>
</td></tr>

<tr><td width='50%' valign='top'>
Ну и до кучи добавил возможнось ввода произвольных строк (это просто текстовой конфиг с кастомной раскладной, кнопка <b>Mod</b> показывает список доступных конфигов):<br>
</td><td width='50%' valign='top'>
There is also a feature which allows entering any predefined strings instead of single characters. (An example of such configuration is shown on the picture. The key <b>Mod</b> shows list of available configuration files).<br>
</td></tr>

<tr><td align='center'>
<img width='480' height='300' src='http://eurokbd.googlecode.com/files/screenshot3.jpg' /><br />
</td></tr>

<tr><td width='50%' valign='top'>
Текущего времени и содержимого клипборда:<br>
</td><td width='50%' valign='top'>
...current time and content of the clipboard.<br>
</td></tr>

<tr><td align='center'>
<img width='480' height='300' src='http://eurokbd.googlecode.com/files/screenshot4.jpg' /><br />
</td></tr>

<tr><td width='50%' valign='top'>
Да, координаты кнопок и раскладки - они в текстовых конфигах, можно исправлять прямо на кпк. Графический конфигуратор конечно было бы прикольно, но вряд ли стоит его делать?<br>
</td><td width='50%' valign='top'>
The coordinates of all buttons and characters/symbols/strings they correspond are stored in text format configuration files. Of course some GUI configurator would look nice, but is it really necessary?<br>
</td></tr>

<tr><td width='50%' valign='top'>
TODO примерно такое (только вряд ли в ближайшее время оно будет делаться):<br />
1. Диалог для конфигурации параметров независимых от раскладки/скина: таймингы автоповтора, включение-выключение групп символов без редактирования раскладки (например греческий или македонская кириллица мало кому нужны)<br />
</td><td width='50%' valign='top'>
TODO list (roughly):<br />
1. Configuration dialog to set up global variables independed from config files (common for all configurations): autorepeat times, enabling-disabling groups of symbol without editing configuration files (for example, disable Macedonian and Greek).<br>
</td></tr>

<tr><td width='50%' valign='top'>
2. Шаблоны strftime для кнопок вставки времени<br />
3. Кнопки Cut, Copy и PrintScreen<br />
4. charmap для ввода символов, кот-е нужны редко, такие символы будет можно смело выкинуть с самой клавиатуры и она будет попроще выглядеть, а если эти символы понадобятся - то их можно вводить через charmap<br />
5. арабский, тайский и иврит<br />
6. подержка кнопок состоящих из 2-х прямоугольников (кривой Enter), возможно еще и 6-угольных кнопок, по ним легче целиться.<br>
</td><td width='50%' valign='top'>
2. strftime-like templates for inserting current time.<br />
3. Keys Cut, Copy and PrintScreen<br />
4. Utility like Windows' charmap.exe. It allows to remove rarely used keys from popups, removed characters could be entered via the charmap.<br />
5. Arabic, Thai, Hebrew languages support.<br />
6. More complex button shapes than simple rectangle. For example curved Enter key, or maybe hexagonal keys for easier targeting.<br>
</td></tr>

</table>
