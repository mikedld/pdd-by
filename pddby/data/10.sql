-- create schema --------------------------------------------------------------
CREATE TABLE `settings` (`key` TEXT, `value` TEXT);
CREATE TABLE `images` (`name` TEXT, `data` BLOB);
CREATE TABLE `comments` (`number` INT, `text` TEXT);
CREATE TABLE `traffregs` (`number` INT, `text` TEXT);
CREATE TABLE `images_traffregs` (`image_id` INT, `traffreg_id` INT);
CREATE TABLE `sections` (`name` TEXT, `title_prefix` TEXT, `title` TEXT);
CREATE TABLE `topics` (`number` TEXT, `title` TEXT);
CREATE TABLE `questions` (`topic_id` INT, `text` TEXT, `image_id` INT, `advice` TEXT, `comment_id` INT);
CREATE TABLE `questions_sections` (`question_id` INT, `section_id`);
CREATE TABLE `questions_traffregs` (`question_id` INT, `traffreg_id`);
CREATE TABLE `answers` (`question_id` INT, `text` TEXT, `is_correct` INT);

-- bootstrab `settings` table -------------------------------------------------
INSERT INTO `settings` VALUES ("ticket_topics_distribution", "1:2:2:2:1:1:1");
INSERT INTO `settings` VALUES ("image_dirs", "image_1:image_2:image_3:image_4:image_5:image_6:signs");

-- bootstrab `sections` table -------------------------------------------------
INSERT INTO `sections` VALUES ("1", "Глава 1", "Общие положения");
INSERT INTO `sections` VALUES ("2", "Глава 2", "Общие права и обязанности участников дорожного движения");
INSERT INTO `sections` VALUES ("3", "Глава 3", "Права и обязанности водителей");
INSERT INTO `sections` VALUES ("4", "Глава 4", "Права и обязанности пешеходов");
INSERT INTO `sections` VALUES ("5", "Глава 5", "Обязанности пассажиров");
INSERT INTO `sections` VALUES ("6", "Глава 6", "Обязанности водителей и других лиц в особых случаях");
INSERT INTO `sections` VALUES ("7", "Глава 7", "Сигналы регулировщика и светофоров");
INSERT INTO `sections` VALUES ("8", "Глава 8", "Применение аварийной световой сигнализации, знака аварийной остановки, фонаря с мигающим красным цветом");
INSERT INTO `sections` VALUES ("9", "Глава 9", "Маневрирование");
INSERT INTO `sections` VALUES ("10", "Глава 10", "Расположение транспортных средств на проезжей части дороги");
INSERT INTO `sections` VALUES ("11", "Глава 11", "Скорость движения транспортных средств");
INSERT INTO `sections` VALUES ("12", "Глава 12", "Обгон, встречный разъезд");
INSERT INTO `sections` VALUES ("13", "Глава 13 общ", "Проезд перекрёстков (п.п. 100...102)");
INSERT INTO `sections` VALUES ("13_1", "Глава 13 ч.1", "Проезд перекрёстков (регулируемые перекрёстки)");
INSERT INTO `sections` VALUES ("13_2", "Глава 13 ч.2", "Проезд перекрёстков (нерегулируемые перекрёстки)");
INSERT INTO `sections` VALUES ("14", "Глава 14", "Пешеходные переходы и остановочные пункты маршрутных транспортных средств");
INSERT INTO `sections` VALUES ("15", "Глава 15", "Преимущество маршрутных транспортных средств");
INSERT INTO `sections` VALUES ("16", "Глава 16", "Железнодорожные переезды");
INSERT INTO `sections` VALUES ("17", "Глава 17", "Движение по автомагистрали");
INSERT INTO `sections` VALUES ("18", "Глава 18", "Движение в жилой и пешеходной зонах, на прилегающей территории");
INSERT INTO `sections` VALUES ("19", "Глава 19", "Остановки и стоянка транспортных средств");
INSERT INTO `sections` VALUES ("20", "Глава 20", "Движение на велосипедах и мопедах");
INSERT INTO `sections` VALUES ("21", "Глава 21", "Движение гужевых транспортных средств, всадников и прогон скота");
INSERT INTO `sections` VALUES ("22", "Глава 22", "Пользование внешними световыми приборами и звуковыми сигналами транспортных средств");
INSERT INTO `sections` VALUES ("23", "Глава 23", "Перевозка пассажиров");
INSERT INTO `sections` VALUES ("24", "Глава 24", "Перевозка грузов");
INSERT INTO `sections` VALUES ("25", "Глава 25", "Буксировка механических транспортных средств");
INSERT INTO `sections` VALUES ("26", "Глава 26", "Основные положения о допуске транспортных средств к участию в дорожном движении, их техническое состояние, оборудование");
INSERT INTO `sections` VALUES ("P2_1", "Приложение 2 ч.1", "Предупреждающие знаки");
INSERT INTO `sections` VALUES ("P2_2", "Приложение 2 ч.2", "Знаки приоритета");
INSERT INTO `sections` VALUES ("P2_3", "Приложение 2 ч.3", "Запрещающие знаки");
INSERT INTO `sections` VALUES ("P2_4", "Приложение 2 ч.4", "Предписывающие знаки");
INSERT INTO `sections` VALUES ("P2_5", "Приложение 2 ч.5", "Информационно-указательные знаки");
INSERT INTO `sections` VALUES ("P2_6", "Приложение 2 ч.6", "Знаки сервиса");
INSERT INTO `sections` VALUES ("P2_7", "Приложение 2 ч.7", "Знаки дополнительной информации (таблички)");
INSERT INTO `sections` VALUES ("P3", "Приложение 3", "Дорожная разметка");
INSERT INTO `sections` VALUES ("P4", "Приложение 4", "Перечень неисправностей транспортных средств и условий, при которых запрещается их участие в дорожном движении");
INSERT INTO `sections` VALUES ("P5", "Приложение 5", "Опознавательные знаки транспортных средств");
INSERT INTO `sections` VALUES ("O", "Ответственность", "Правовые основы дорожного движения");
INSERT INTO `sections` VALUES ("B", "Безопасность", "Основы управления транспортным средством и безопасность");
INSERT INTO `sections` VALUES ("M", "Медицина", "Доврачебная медицинская помощь пострадавшим при ДТП");

-- bootstrab `topics` table ---------------------------------------------------
INSERT INTO `topics` VALUES ("1", "Главы 1-6");
INSERT INTO `topics` VALUES ("2", "Дорожные знаки и разметка. Приложения 2-3");
INSERT INTO `topics` VALUES ("3", "Главы 7, 13. Приложение 1");
INSERT INTO `topics` VALUES ("4", "Главы 8-12 и 19");
INSERT INTO `topics` VALUES ("5", "Главы 14-18, 20-25");
INSERT INTO `topics` VALUES ("6", "Глава 26. Приложение 4");
INSERT INTO `topics` VALUES ("7", "Ответственность. Безопасность. Медицина");
