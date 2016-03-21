-- -*- coding: utf-8;-*-
-- Global preloads

-- This file uses utf-8 coding. 

print("Global preload")

-- Global UI text
text =
{
	-- English
	en =
	{
		ui =
		{
			player_caption = "Player:",
			mountain_caption = "Mountain:",

			main_menu = "MAIN MENU",
			change_player = "Change Player",
			change_mountain = "Change Mountain",
			run = "Run",
			carve_now = "Carve Now!",
			vcr = "VCR",
			heli_drop = "Heli-Drop",
			options = "Options",
			credits = "Credits",
			quit = "Quit",
	
			quit_menu = "QUIT?",
			quit_yes = "Yes",
			quit_no = "No",

			select_mountain = "SELECT MOUNTAIN",
			cancel = "    Cancel    ",

			choose_a_run = "     Choose a run:                            ",

			restart_run = "Restart Run",
			next_run = "Next Run",
			exit = "Exit",
			rewind = "Rewind & Try Again",

			change_player = "Change Player",
			change_mountain = "Change Mountain",

			options_menu = "OPTIONS",
			display = "Display",
			sound = "Sound",
			weather = "Weather",

			sound_menu = "SOUND",
			master_volume = "Master Volume: ",
			sfx_volume = "SFX Volume: ",
			cd_audio_volume = "CD-Audio Volume: ",

			display_menu_caption = "DISPLAY",
			terrain_mesh_detail = "Terrain mesh detail: ",
			texture_resolution = "Texture resolution: ",
			texture_build_limit = "Texture build limit: ",
			detail_texturing = "Detail texturing: ",
			clouds = "Clouds: ",
			distance_haze = "Distance haze: ",
			display_frame_rate = "Display frame rate: ",

			weather_menu = "WEATHER",
			clear = "Clear",
			partly_cloudy = "Partly cloudy",
			sunset = "Sunset",
			snowing = "Snowing",
			whiteout = "Whiteout",
			choose_a_recording = "     Choose a recording     ",
			enter_recording_name = "  Enter Recording Name  ",
			new_player = "New Player",
			load_player = "Load Player",

			choose_player_menu = "CHOOSE PLAYER",
			enter_player_name = "  Enter Player Name  ",

			run_info_caption = "Run Info:",
			heli_drop_name = "heli-drop",
			heli_drop_desc = "You're on your own.  Good luck...",
			par_time_caption = "Par Time:",
			your_time_caption = "Your Time",
			run_drop_caption = "Vertical drop:",
			your_high_caption = "Your High",
			local_high_caption = "Local High",
			scoresheet_caption = "Scoresheet",
			difference_caption = "Difference",
			high_scores_caption = "High Scores",
			yours_caption = "Yours",
			local_caption = "Local",

			toe_side_grab_bonus = "toe side grab +100",
			heel_side_grab_bonus = "heel side grab +100",
			tail_grab_bonus = "tail grab +100",
			air_bonus = "air",
			nice_air_bonus = "nice air",
			big_air_bonus = "big air",
			tremendous_air_bonus = "tremendous air",
			degrees_bonus = "degrees",
			flip_bonus = "flip",

			attract_feature1 = "no visibility limits",
			attract_feature2 = "physics-based gameplay",
			attract_feature3 = "real world locations",
			attract_feature4 = "find your own route",

			outro1 = "For hints, upgrades, high scores and information on \n" ..
					"the revolutionary Catapult snowboard controller, visit",
			outro2 = "www.soulride.com ",

			get_ready_prompt = "Get Ready...",
			ready_prompt = "Ready",
			set_prompt = "Set",
			go_prompt = "Go!",

			crash_message = "CRASH",
			finished_message = "FINISHED",

			loading_caption = "loading...",
			surface_textures_caption = "surface textures",
			topography_caption = "topography",
			terrain_coverage_caption = "terrain coverage",
			models_caption = "models",
			objects_caption = "objects",
			regions_caption = "regions",

			credits_rights_reserved = "Copyright 2003\nAll rights reserved",

			-- char 169 is the copyright symbol in Windows; don't encode it with utf-8 because
			-- we display this string directly in a win32 string control
			win_copyright = "\169 2003\nSlingshot Game Technology",
			win_start_game = "Start Game",
			win_display_options = "Display Options...",

			win_display_options_caption = "Display Options",
			win_fullscreen = "Fullscreen",
			win_desired_mode = "Desired Mode",
			win_opengl_driver = "OpenGL Driver",
			win_add = "Add...",
			win_delete = "Delete",
			win_32_bit_textures = "32-bit textures",
			win_edit = "Edit...",
			win_options = "Options",
			win_dll_filename = "DLL Filename",

			win_driver_info_caption = "Driver Information",
			win_ok = "OK",
			win_cancel = "Cancel",
			win_driver_dll_file = "Driver DLL File",
			win_comment = "Comment",
			win_browse = "Browse...",

			win_choose_opengl_dll = "Choose an OpenGL .DLL",
			win_dll_files = "DLL files",
		},
	},
	
	-- Italian
	it =
	{
		ui =
		{
			player_caption = "Giocatore:",
			mountain_caption = "Montagna:",

			main_menu = "MENU PRINCIPALE",
			change_player = "Cambia giocatore",
			change_mountain = "Cambia montagna",
			run = "Discesa",
			carve_now = "Parti ora!",
			vcr = "Registrazione",
			heli_drop = "Elicottero",
			options = "Opzioni",
			credits = "Ringraziamenti",
			quit = "Esci",
	
			quit_menu = "ESCI?",
			quit_yes = "Sì",
			quit_no = "No",

			select_mountain = "SCEGLI MONTAGNA",
			cancel = "    Annulla    ",

			choose_a_run = "     Scegli una discesa:                            ",

			restart_run = "Ricomincia",
			next_run = "Discesa successiva",
			exit = "Esci",
			rewind = "Riavvolgi e riprova",

			change_player = "Cambia Giocatore",
			change_mountain = "Cambia Montagna",

			options_menu = "OPZIONI",
			display = "Schermo",
			sound = "Audio",
			weather = "Clima",

			sound_menu = "AUDIO",
			master_volume = "Volume principale: ",
			sfx_volume = "Volume effetti audio: ",
			cd_audio_volume = "Volume audio CD: ",

			display_menu_caption = "SCHERMO",
			terrain_mesh_detail = "Dettaglio terreno: ",
			texture_resolution = "Risoluzione trama: ",
			texture_build_limit = "Limite creazione trame: ",
			detail_texturing = "Trame dettagli: ",
			clouds = "Nuvole: ",
			distance_haze = "Foschia a distanza: ",
			display_frame_rate = "Visualizza fps: ",

			weather_menu = "CLIMA",
			clear = "Sereno",
			partly_cloudy = "Parzialmente nuvoloso",
			sunset = "Tramonto",
			snowing = "Neve",
			whiteout = "Neve e scarsa visibilità",
			choose_a_recording = "     Scegli una registrazione     ",
			enter_recording_name = "  Nome registrazione  ",
			new_player = "Nuovo giocatore",
			load_player = "Carica giocatore",

			choose_player_menu = "SCEGLI GIOCATORE",
			enter_player_name = "  Nome giocatore  ",

			run_info_caption = "Info discesa:",
			heli_drop_name = "Elicottero",
			heli_drop_desc = "La pista è tutta tua.  Buona fortuna...",
			par_time_caption = "Tempo massimo:",
			your_time_caption = "Tempo registrato",
			run_drop_caption = "Dislivello verticale:",
			your_high_caption = "Punteggio",
			local_high_caption = "Record punteggio",
			scoresheet_caption = "Punteggi",
			difference_caption = "Differenza",
			high_scores_caption = "High Scores",
			yours_caption = "Personale",
			local_caption = "Record",

			toe_side_grab_bonus = "grab su lamina toeside +100",
			heel_side_grab_bonus = "grab su lamina heelside +100",
			tail_grab_bonus = "tail grab +100",
			air_bonus = "air",
			nice_air_bonus = "nice air",
			big_air_bonus = "big air",
			tremendous_air_bonus = "tremendous air",
			degrees_bonus = "gradi",
			flip_bonus = "flip",

			attract_feature1 = "Nessun limite di visibilità",
			attract_feature2 = "Realismo nei movimenti",
			attract_feature3 = "Località realmente esistenti",
			attract_feature4 = "Percorsi personalizzati",

			outro1 = "For hints, upgrades, high scores and information on \n" ..
					"the revolutionary Catapult snowboard controller, visit",
			outro2 = "www.soulride.com ",

			get_ready_prompt = "Preparativi...",
			ready_prompt = "Pronto",
			set_prompt = "Imposta",
			go_prompt = "Vai!",

			crash_message = "CADUTA",
			finished_message = "FINE",

			loading_caption = "Caricamento in corso...",
			surface_textures_caption = "Trame superficie",
			topography_caption = "Topografia",
			terrain_coverage_caption = "Copertura terreno",
			models_caption = "Modelli",
			objects_caption = "Oggetti",
			regions_caption = "Regioni",

			credits_rights_reserved = "Copyright 2003\nTutti i diritti riservati",

			win_copyright = "\169 2003\nSlingshot Game Technology",
			win_start_game = "Avvia gioco",
			win_display_options = "Opzioni",

			win_display_options_caption = "Opzioni di visualizzazione...",
			win_fullscreen = "Schermo intero",
			win_desired_mode = "Modalit�",
			win_opengl_driver = "Driver OpenGL",
			win_add = "Aggiungi",
			win_delete = "Elimina",
			win_32_bit_textures = "Trama a 32 bit",
			win_edit = "Modifica",
			win_options = "Opzioni",
			win_dll_filename = "Nome DLL",

			win_driver_info_caption = "Informazioni sul driver",
			win_ok = "OK",
			win_cancel = "Annulla",
			win_driver_dll_file = "File driver DLL",
			win_comment = "Commento",
			win_browse = "Sfoglia...",

			win_choose_opengl_dll = "Seleziona un file DLL OpenGL",
			win_dll_files = "File DLL",
		},
	},


	-- Samples of special chars, UTF-8 encoding
	-- "������ĄąĆćĘę",
	-- "ŁłŃńÓóŚśŹźŻż",

	-- Polish
	pl =
	{
		ui =
		{
			player_caption = "Gracz:",
			mountain_caption = "Góra:",

			main_menu = "MENU GŁÓWNE",
			change_player = "Zmień gracza",
			change_mountain = "Zmień górę",
			run = "Trasa",
			carve_now = "Start!",
			vcr = "Kamera",
			heli_drop = "Heli-Zrzut",
			options = "Opcje",
			credits = "Autorzy",
			quit = "Wyjście",
	
			quit_menu = "Wyjście?",
			quit_yes = "Tak",
			quit_no = "Nie",

			select_mountain = "WYBIERZ GÓRĘ",
			cancel = "    Anuluj    ",

			choose_a_run = "     Wybierz trasę:                           ",

			restart_run = "Zacznij od początku",
			next_run = "Następna trasa",
			exit = "Wyjście",
			rewind = "Przewiń i spróbuj ponownie",

			change_player = "Zmien gracza",
			change_mountain = "Zmien góre",

			options_menu = "OPCJE",
			display = "Grafika",
			sound = "Dźwięk",
			weather = "Pogoda",

			sound_menu = "DŹWIĘK",
			master_volume = "Głośność: ",
			sfx_volume = "Głośność efektów: ",
			cd_audio_volume = "Głośność muzyki: ",

			display_menu_caption = "GRAFIKA",
			terrain_mesh_detail = "Szczegóły terenu: ",
			texture_resolution = "Rozdzielczość tekstur: ",
			texture_build_limit = "Dystans zmiany detali: ",
			detail_texturing = "Szczegółowe tekstury: ",
			clouds = "Chmury: ",
			distance_haze = "Mgła atmosferyczna: ",
			display_frame_rate = "Licznik klatek/sek: ",

			weather_menu = "POGODA",
			clear = "Bezchmurnie",
			partly_cloudy = "Lekkie zachmurzenie",
			sunset = "Zachód słońca",
			snowing = "Opady śniegu",
			whiteout = "Zamieć śnieżna",
			choose_a_recording = "     Wybierz klip     ",
			enter_recording_name = "   Podaj nazwę klipu  ",
			new_player = "Nowy gracz",
			load_player = "Załaduj gracza",

			choose_player_menu = "WYBIERZ GRACZA",
			enter_player_name = "  Podaj imię gracza  ",

			run_info_caption = "Informacje:",
			heli_drop_name = "heli-zrzut",
			heli_drop_desc = "Teraz możesz liczyć tylko na siebie. Powodzenia...",
			par_time_caption = "Norma czasowa:",
			your_time_caption = "Twój czas",
			run_drop_caption = "Spadek trasy:",
			your_high_caption = "Twój rekord",
			local_high_caption = "Rekord",
			scoresheet_caption = "Wynik",
			difference_caption = "Różnica",
			high_scores_caption = "Rekordy",
			yours_caption = "Twoje",
			local_caption = "Lokalne",

			toe_side_grab_bonus = "chwyt przednia krawędź + 100",
			heel_side_grab_bonus = "chwyt tylna krawędź + 100",
			tail_grab_bonus = "chwyt ogon + 100",
			air_bonus = "skok",
			nice_air_bonus = "niezły skok",
			big_air_bonus = "duży skok",
			tremendous_air_bonus = "wielki skok",
			degrees_bonus = "stopni",
			flip_bonus = "salto",

			attract_feature1 = "Widoczność bez ograniczeń",
			attract_feature2 = "Realistyczna fizyka jazdy",
			attract_feature3 = "Rzeczywiste trasy zjazdowe",
			attract_feature4 = "Znajdź swoją własną trasę",

			outro1 = "Zapraszamy na strony Internetowe\n" ..
					"www.soulride.com",
			outro2 = "www.topware.pl",

			get_ready_prompt = "Przygotuj się...",
			ready_prompt = "Do startu...",
			set_prompt = "Gotów...",
			go_prompt = "Start!",

			crash_message = "GLEBA",
			finished_message = "UKOŃCZONY",

			loading_caption = "Ładowanie...",
			surface_textures_caption = "tekstury",
			topography_caption = "topografia",
			terrain_coverage_caption = "teren",
			models_caption = "modele",
			objects_caption = "obiekty",
			regions_caption = "regiony",

			credits_rights_reserved = "Copyright 2003\nWszelkie prawa zastrzeżone",

			win_copyright = "\169 2003\nSlingshot Game Technology",
			win_start_game = "Start",
			win_display_options = "Ustawienia grafiki",

			win_display_options_caption = "Ustawienia grafiki",
			win_fullscreen = "Pełny ekran",
			win_desired_mode = "Tryb graficzny",
			win_opengl_driver = "Sterownik OpenGL",
			win_add = "Dodaj...",
			win_delete = "Usuń",
			win_32_bit_textures = "Tekstury 32-bitowe",
			win_edit = "Edycja...",
			win_options = "Opcje",
			win_dll_filename = "Nazwa pliku DLL",

			win_driver_info_caption = "Informacje o sterowniku",
			win_ok = "OK",
			win_cancel = "Anuluj",
			win_driver_dll_file = "Plik DLL sterownika",
			win_comment = "Komentarz",
			win_browse = "Wskaż...",
		},
	},

	-- German
	de =
	{
		ui =
		{
			player_caption = "Spieler:",
			mountain_caption = "Gebirge:",

			main_menu = "HAUPTMENÜ",
			change_player = "Spieler wählen",
			change_mountain = "Gebirge ändern",
			run = "Route",
			carve_now = "Jetzt carven!",
			vcr = "Videorekorder",
			heli_drop = "Heli-Drop",
			options = "Optionen",
			credits = "Über",
			quit = "Beenden",
	
			quit_menu = "BEENDEN?",
			quit_yes = "Ja",
			quit_no = "Nein",

			select_mountain = "GEBIRGE WÄHLEN",
			cancel = "   Abbrechen  ",

			choose_a_run = "   Wählen Sie eine Abfahrt aus:          ",

			restart_run = "Abfahrt neu starten",
			next_run = "Nächste Abfahrt",
			exit = "Beenden",
			rewind = "Zurück und erneut versuchen",

			change_player = "Spieler ändern",
			change_mountain = "Gebirge ändern",

			options_menu = "OPTIONEN",
			display = "Ansicht",
			sound = "Sound",
			weather = "Wetter",

			display_menu_caption = "ANSICHT",
			terrain_mesh_detail = "Terrain Details: ",
			texture_resolution = "Textur Auflösung: ",
			texture_build_limit = "Textur Beschränkung: ",
			detail_texturing = "Detail Textur: ",
			clouds = "Wolken: ",
			distance_haze = "Nebel: ",
			display_frame_rate = "Frame Rate anzeigen: ",

			weather_menu = "WETTER",
			clear = "Klar",
			partly_cloudy = "Leicht bewölkt",
			sunset = "Sonnenuntergang",
			snowing = "Schnee",
			whiteout = "Ohne",
			choose_a_recording = "      Eine Aufnahme auswählen  ",
			enter_recording_name = "Einen Namen für die Aufnahme festlegen",
			new_player = "Neuen Spieler anlegen",
			load_player = "Spieler laden",

			choose_player_menu = "SPIELER WÄHLEN",
			enter_player_name = "Name des Spielers",

			run_info_caption = "Info:",
			heli_drop_name = "heli-drop",
			heli_drop_desc = "Sie befinden sich nun an der gewünschten Stelle im Gebirge. Viel Glück!",
			par_time_caption = "Vorgabezeit:",
			your_time_caption = "Ihre Zeit",
			run_drop_caption = "Höhenunterschied:",
			your_high_caption = "Ihre Punkte",
			local_high_caption = "Streckenrekord",
			scoresheet_caption = "Punkte",
			difference_caption = "Unterschied",
			high_scores_caption = "Punkte",
			yours_caption = "Ihre",
			local_caption = "Rekord",

			toe_side_grab_bonus = "Vorderseitiger Grab +100",
			heel_side_grab_bonus = "Rückseitiger Grab +100",
			tail_grab_bonus = "Tail Grab +100",
			air_bonus = "Sprung",
			nice_air_bonus = "Guter Sprung",
			big_air_bonus = "Grosser Sprung",
			tremendous_air_bonus = "Fantastischer Sprung",
			degrees_bonus = "Grad",
			flip_bonus = "Drehung",

			attract_feature1 = "Keine Sicht-Beschränkungen",
			attract_feature2 = "Realistisches Spiel",
			attract_feature3 = "Echte Gebiete",
			attract_feature4 = "Finden Sie Ihre eigene Route",

			outro1 = "Weitere Informationen finden Sie unter",
			outro2 = "www.purplehills.de ",

			get_ready_prompt = "Starten...",
			ready_prompt = "Auf die Plätze",
			set_prompt = "Fertig",
			go_prompt = "Los!",

			crash_message = "CRASH",
			finished_message = "BEENDET",

			loading_caption = "Laden...",
			surface_textures_caption = "Oberfläche Texturen",
			topography_caption = "Topographie",
			terrain_coverage_caption = "Terrain Reichweite",
			models_caption = "Modelle",
			objects_caption = "Objekte",
			regions_caption = "Regionen",

			win_copyright = "\169 2003\nSlingshot Game Technology",
			win_start_game = "Spiel starten",
			win_display_options = "Optionen...",

			win_display_options_caption = "Optionen",
			win_fullscreen = "Vollbild",
			win_desired_mode = "Modus",
			win_opengl_driver = "OpenGL Treiber",
			win_add = "Hinzufügen...",
			win_delete = "Löschen",
			win_32_bit_textures = "32-Bit Textur",
			win_edit = "Bearbeiten...",
			win_options = "Bearbeiten",
			win_dll_filename = "DLL Dateiname",

			win_driver_info_caption = "Treiber Information",
			win_ok = "OK",
			win_cancel = "Abbrechen",
			win_driver_dll_file = "Treiber DLL Datei",
			win_comment = "Anmerkung",
			win_browse = "Durchsuchen...",
		},
	},
}

