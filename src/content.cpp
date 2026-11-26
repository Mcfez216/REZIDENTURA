// content.cpp - the four postings of the career: London, Vienna, Washington, Bonn.
// All people and agents are invented. Places and the historical backdrop are real.
// To add content (jobs, names, requirements, districts, events) this is the only file you need to edit.
#include "content.h"
#include "core.h"
#include <initializer_list>

template <class T, size_t N> static void fill(T (&dst)[N], std::initializer_list<T> src) {
    size_t i = 0;
    for (const T& x : src) if (i < N) dst[i++] = x;
}
static void streets(Theatre& t, std::initializer_list<const char*> s) {
    size_t i = 0;
    for (const char* x : s) { if (i < 24) t.street[i / 3][i % 3] = x; i++; }
}

const std::vector<const char*> CODENAMES = {
    "GRANIT", "KORAL", "LESNIK", "TENOR", "SOKOL", "DROZD", "BEREG", "YAKOR", "KVARTS", "LIRA", "MAYAK", "TURIST", "KEDR", "GORN",
    "VESNA", "AKKORD", "ROMB", "ZIMA", "PILOT", "FAKEL", "BARS", "ILMEN", "ONEGA", "RUBIN", "SATURN", "TOPAZ", "VYMPEL", "ZEFIR",
    "ALMAZ", "BURAN", "VOLNA", "GRANAT", "DUNAI", "YENISEI", "ZARYA", "IRTYSH", "KASKAD", "LADOGA", "MERKURI", "NEVA", "OKEAN",
    "PAMIR", "RADUGA", "SELENA", "TAIGA", "URAL", "FREGAT", "KHRUSTAL", "TSIKLON", "SHTORM", "ELBRUS", "YUPITER", "YANTAR",
    "ARKTUR", "BARKHAN", "VEKTOR", "GEIZER", "DELTA", "YEGER", "ZHEMCHUG", "AGAT", "BERKUT", "VITYAZ", "GRAD", "DESNA", "KOMETA",
    "LOTOS", "MIRAZH", "NARZAN", "ORION", "PLAMYA", "RITM", "SIGNAL", "TAIFUN", "UTYOS", "FOTON", "KHIBINY", "CHAIKA", "SHKVAL", "ETALON"};

const std::vector<const char*> RU_FIRST = {"Viktor", "Nikolai", "Yuri", "Anatoly", "Gennady", "Boris", "Valery", "Leonid", "Igor", "Sergei",
    "Aleksandr", "Yevgeny", "Stanislav", "Vadim", "Konstantin", "Pavel", "Mikhail", "Arkady", "Lev", "Fyodor"};
const std::vector<const char*> RU_LAST = {"Orlov", "Belov", "Titov", "Zaitsev", "Morozov", "Volkov", "Lebedev", "Kozlov", "Novikov", "Fedorov",
    "Golubev", "Vinogradov", "Bogdanov", "Vorobyov", "Sorokin", "Baranov", "Kulikov", "Maksimov", "Gusev", "Kiselyov", "Ilyin",
    "Gerasimov", "Tarasov", "Danilov", "Zhuravlev", "Savelyev", "Rybakov", "Nikitin", "Sobolev", "Antonov", "Filippov", "Yermakov",
    "Shubin", "Dyomin", "Loginov", "Krylov"};

// ============================================================================ LONDON 1968-1971
static Theatre london() {
    Theatre t{};
    t.city = "London"; t.country = "the United Kingdom";
    t.cover = "Third Secretary (Press)"; t.embassy = "Embassy of the USSR, 13 Kensington Palace Gardens, London W8";
    t.adversary = "the Security Service"; t.watchers = "A4"; t.currency = "pounds"; t.chief = "Rezident";
    t.lang = 0;
    t.sy = 1968; t.sm = 9; t.sd = 2; t.ey = 1971; t.em = 12; t.ed = 31;
    t.role = 0; t.subs = 0; t.colleagues = 6;
    t.price = 1; t.a4Base = 0.85f; t.heat0 = 14; t.blownHeat = 0; t.dangle = 0.10f; t.walkIn = 0.35f; t.valuePar = 900;
    fill(t.district, {"Kensington", "Westminster", "Soho", "Bloomsbury", "Hampstead", "Islington", "Greenwich", "Richmond"});
    fill(t.dist, {0, 1, 1, 1, 2, 2, 3, 3});
    fill(t.density, {0.30f, 0.25f, 0.13f, 0.12f, 0.12f, 0.07f, 0.05f, 0.05f});
    streets(t, {"Kensington Church Street", "Holland Street", "Campden Hill Road", "Horseferry Road", "Marsham Street", "Strutton Ground",
                "Dean Street", "Wardour Street", "Berwick Street", "Lamb's Conduit Street", "Marchmont Street", "Museum Street",
                "Flask Walk", "South End Road", "Heath Street", "Upper Street", "Essex Road", "Amwell Street",
                "Crooms Hill", "Royal Hill", "Trafalgar Road", "Kew Road", "Hill Rise", "Sheen Road"});
    fill(t.park, {"Holland Park", "St James's Park", "Soho Square", "Coram's Fields", "Hampstead Heath", "Highbury Fields", "Greenwich Park", "Kew Green"});
    fill(t.station, {"Notting Hill Gate station", "Victoria station", "Tottenham Court Road station", "Russell Square station",
                     "Belsize Park station", "Angel station", "Maze Hill station", "Kew Gardens station"});
    t.venue = {"The Plough", "The Rising Sun", "The Coach & Horses", "The Lamb", "The Wheatsheaf", "The Three Tuns", "The Crown",
               "The White Hart", "The Feathers", "The Bricklayers Arms"};
    t.meetT = {"%P, %S", "tea-room, %S", "Italian restaurant, %S", "cinema foyer, %S", "saloon bar of the hotel, %S", "bench walk, %K"};
    t.dropT = {"loose brick, churchyard wall off %S", "hollow behind the third bench, %K", "behind the cistern, gents' lavatory by %T",
               "magnetic container under railing, %S", "drainpipe bracket, mews off %S", "under the marked stone, footpath in %K"};
    t.signalT = {"chalk mark, lamp-post, %S", "drawing-pin, bench in %K", "strip of tape, railing outside %T", "chalk mark, pillar-box, %S"};
    t.male = {"Alan", "Arthur", "Brian", "Colin", "David", "Derek", "Dennis", "Edward", "Frank", "Geoffrey", "Gordon", "Harold", "Ian",
              "John", "Keith", "Kenneth", "Leonard", "Malcolm", "Michael", "Norman", "Peter", "Raymond", "Roger", "Ronald", "Stanley",
              "Terence", "Trevor", "Victor", "Walter", "William"};
    t.female = {"Anne", "Barbara", "Christine", "Doreen", "Eileen", "Jean", "Margaret", "Patricia", "Sheila", "Valerie"};
    t.last = {"Ashworth", "Baines", "Barlow", "Calder", "Chadwick", "Coombes", "Dacre", "Eckersley", "Fenwick", "Garside", "Hallam",
              "Hartley", "Ibbotson", "Jessop", "Kershaw", "Lomax", "Maddox", "Naylor", "Oakes", "Pickering", "Quayle", "Rudge", "Sowerby",
              "Tetlow", "Underhill", "Varley", "Wainwright", "Yardley", "Bracewell", "Crowther", "Dearden", "Entwistle", "Fairclough",
              "Greaves", "Holroyd", "Ingham", "Kettle", "Lightfoot", "Meakin", "Pennington"};
    t.jobs[POLITICAL] = {
        {"Assistant private secretary", "Foreign & Commonwealth Office", 60, 85, 72, 15, false},
        {"Registry clerk", "Cabinet Office", 65, 90, 82, 5, false},
        {"Desk officer", "Board of Trade", 40, 65, 50, 15, false},
        {"Research officer", "Labour Party, Transport House", 30, 55, 15, 55, false},
        {"Lobby correspondent", "a national newspaper", 25, 50, 8, 85, false},
        {"Research assistant to an MP", "House of Commons", 30, 55, 18, 60, false},
        {"District official", "an engineering trade union", 20, 45, 8, 60, true},
        {"Second secretary", "embassy of a non-aligned state", 35, 60, 30, 35, false},
        {"Economist", "the Treasury", 55, 80, 65, 20, false}};
    t.jobs[MILITARY] = {
        {"Chief petty officer (signals)", "Ministry of Defence (Navy)", 60, 88, 78, 5, true},
        {"Clerical officer", "Ministry of Defence (Air)", 50, 78, 70, 5, false},
        {"Technician", "RAF signals unit", 55, 82, 72, 5, true},
        {"Draughtsman", "a naval shipbuilder's London office", 40, 65, 48, 5, true},
        {"Staff sergeant (movements)", "Ministry of Defence (Army)", 45, 70, 66, 5, true},
        {"Executive officer", "MoD Procurement Executive", 55, 82, 70, 10, false},
        {"Defence correspondent", "a weekly journal", 25, 45, 10, 75, false}};
    t.jobs[SCITECH] = {
        {"Propulsion engineer", "a Bristol aero-engine works", 55, 85, 58, 10, true},
        {"Laboratory technician", "a London college", 20, 45, 8, 10, false},
        {"Electronics engineer", "an avionics firm", 50, 78, 52, 10, false},
        {"Process chemist", "a chemicals combine", 35, 60, 35, 10, false},
        {"Systems programmer", "a British computer maker", 40, 68, 35, 10, false},
        {"Research fellow", "Atomic Energy Authority", 65, 90, 84, 25, false},
        {"Sales engineer", "a telecommunications firm", 35, 62, 38, 10, false},
        {"Patent agent's clerk", "a Chancery Lane practice", 30, 52, 10, 5, false}};
    t.jobs[SECURITY] = {
        {"Registry clerk", "Metropolitan Police Special Branch", 60, 85, 80, 5, false},
        {"Telephone engineer", "General Post Office", 40, 68, 45, 5, true},
        {"Immigration officer", "Home Office", 35, 60, 50, 5, false},
        {"Clerk, motor licensing", "Greater London Council", 30, 55, 12, 5, false},
        {"Security officer", "a government research establishment", 50, 75, 70, 5, true},
        {"Private enquiry agent", "self-employed (ex-police)", 25, 50, 5, 15, true}};
    t.reqs = {
        {POLITICAL, "British line on Czechoslovakia", "Documentary material on HMG and NATO reactions to the August events in the CSSR; any planned measures against the socialist states.", 0, 110, 120, RK_COLLECT},
        {MILITARY, "NATO strategy MC 14/3", "British implementing plans for the 'flexible response' concept; nuclear release procedures.", 0, 200, 0, RK_COLLECT},
        {SCITECH, "Olympus 593 engine", "Technical documentation on the Olympus 593 supersonic transport engine: turbine blade cooling, materials, test results.", 0, 220, 0, RK_COLLECT},
        {SECURITY, "Surveillance service of the adversary", "Vehicles, radio procedure, observation posts and working methods of the Security Service watchers in London.", 0, 140, 0, RK_COLLECT},
        {MILITARY, "Polaris force", "Patrol cycle, refit schedule and communications of the Royal Navy ballistic missile submarines.", 2, 240, 0, RK_COLLECT},
        {POLITICAL, "Service A: NATO burden-sharing", "ACTIVE MEASURE. Place material through press or parliamentary contacts sharpening British resentment of the cost of the Rhine Army and of American pressure on sterling. Three placements required.", 3, 3, 300, RK_ACTIVE},
        {POLITICAL, "Trade union legislation", "Cabinet differences over industrial relations legislation; attitudes of the union leaderships.", 5, 120, 150, RK_COLLECT},
        {SCITECH, "Carbon-fibre fan blades", "Manufacturing process and test data for carbon-fibre composite compressor blades for large turbofans.", 4, 180, 0, RK_COLLECT},
        {MILITARY, "East of Suez withdrawal", "Timetable, force levels and residual commitments in the Gulf and Far East.", 6, 130, 180, RK_COLLECT},
        {POLITICAL, "European security conference", "FCO assessments of the Budapest appeal; expected British tactics on a European security conference.", 8, 140, 180, RK_COLLECT},
        {SECURITY, "Positive vetting", "Procedures and criteria of security vetting in government departments and defence contractors.", 8, 130, 0, RK_COLLECT},
        {POLITICAL, "Northern Ireland", "Government intentions following the deployment of troops; cabinet-level assessments.", 12, 130, 150, RK_COLLECT},
        {SCITECH, "Third-generation computers", "Design documentation, integrated-circuit supply and software for British mainframe computers.", 10, 200, 0, RK_COLLECT},
        {MILITARY, "BAOR order of battle", "Current order of battle, readiness states and reinforcement plans of the British Army of the Rhine.", 12, 220, 0, RK_COLLECT},
        {POLITICAL, "Opposition foreign policy", "Foreign and defence policy intentions of the Conservative leadership ahead of a general election.", 15, 120, 120, RK_COLLECT},
        {POLITICAL, "Service A: the Common Market", "ACTIVE MEASURE. Place material through press and trade-union contacts on the cost of Common Market entry to British wages and food prices. Three placements required.", 18, 3, 300, RK_ACTIVE},
        {SCITECH, "Gas centrifuge enrichment", "The tripartite centrifuge project: rotor materials, cascade design, separative performance.", 17, 240, 0, RK_COLLECT},
        {MILITARY, "V/STOL strike aircraft", "Service introduction, performance limits and weapon loads of the new vertical take-off aircraft.", 14, 170, 0, RK_COLLECT},
        {POLITICAL, "EEC entry negotiations", "British negotiating positions and fall-back lines in the talks on entry to the Common Market.", 22, 180, 200, RK_COLLECT},
        {SECURITY, "Telephone interception", "Technical and legal arrangements for interception of telephone lines of diplomatic missions.", 18, 150, 0, RK_COLLECT},
        {POLITICAL, "Four-Power Berlin talks", "British position and co-ordination with the US and France in the negotiations on West Berlin.", 20, 150, 180, RK_COLLECT},
        {SCITECH, "Inertial navigation", "Design of inertial navigation platforms and gyroscopes for aircraft and submarines.", 24, 200, 0, RK_COLLECT},
        {MILITARY, "Northern flank exercises", "Plans and after-action reports of NATO naval exercises in the Norwegian Sea.", 24, 160, 150, RK_COLLECT},
        {SECURITY, "Port and airport controls", "Special Branch watch procedures and lists at ports and airports.", 26, 130, 0, RK_COLLECT},
        {POLITICAL, "Policy toward the USSR", "Intentions of the new government in bilateral relations; any measures planned against Soviet establishments.", 30, 160, 0, RK_COLLECT},
        {SCITECH, "Electronic telephone exchanges", "Documentation on stored-program electronic switching equipment.", 28, 170, 0, RK_COLLECT}};
    t.hist = {
        {1968, 10, 27, "Embassy bulletin", "Demonstration in Grosvenor Square", "A very large demonstration against the American war in Vietnam is marching on the US Embassy today. The police and Special Branch are fully committed. KR notes that the watchers will be thin on the ground for some days.", 0, -0.3f, 0, H_NONE},
        {1969, 3, 3, "Embassy bulletin", "Incident on the Ussuri", "Armed clashes with Chinese troops are reported at Damansky Island. Centre will want British assessments of Sino-Soviet relations: any source with Foreign Office access should be asked.", 0, 0, 0, H_NONE},
        {1969, 8, 15, "Embassy bulletin", "Troops in Northern Ireland", "British troops have been deployed on the streets of Londonderry and Belfast. KR expects part of the Security Service's effort to be drawn off to Irish matters.", -0.07f, 0, 0, H_NONE},
        {1970, 6, 19, "Embassy bulletin", "General election", "The Conservatives have won the general election against the forecasts. The Ambassador expects a harder line towards the size of Soviet establishments in London.", 0.05f, 0, 0, H_NONE},
        {1970, 8, 13, "Embassy bulletin", "Moscow Treaty", "The treaty between the USSR and the Federal Republic of Germany was signed in Moscow yesterday. Centre requires British reactions.", 0, 0, 0, H_NONE},
        {1971, 2, 15, "Embassy bulletin", "Decimal currency", "From today the British use a decimal currency. The accounts section asks officers not to submit claims in shillings.", 0, 0, 0, H_NONE},
        {1971, 9, 3, "Rezident", "MOST URGENT - all officers", "", 0.15f, 0, 12, H_DEFECTION},
        {1971, 9, 24, "Rezident", "The expulsions", "", 0.75f, 0, 0, H_FOOT}};
    t.local = "The local surveillance service is stretched but it is not blind. Read the Zenit summary every morning.";
    t.tour = "";
    t.slot = 0; t.cityRu = "ЛОНДОН";
    fill(t.at, {Pt{0.30f, 0.50f}, Pt{0.50f, 0.56f}, Pt{0.49f, 0.43f}, Pt{0.58f, 0.36f}, Pt{0.40f, 0.17f}, Pt{0.64f, 0.23f}, Pt{0.86f, 0.71f}, Pt{0.10f, 0.81f}});
    t.river = {{0.0f, 0.86f}, {0.07f, 0.72f}, {0.20f, 0.68f}, {0.32f, 0.63f}, {0.44f, 0.64f}, {0.53f, 0.62f}, {0.62f, 0.54f}, {0.72f, 0.60f}, {0.80f, 0.63f}, {0.88f, 0.60f}, {1.0f, 0.52f}};
    t.spies = {
        {"Officer, counter-espionage branch", "the Security Service", 75, 95, SVC_SECURITY},
        {"Watcher, mobile surveillance section", "the Security Service", 48, 70, SVC_SECURITY},
        {"Detective sergeant (Soviet section)", "Metropolitan Police Special Branch", 50, 74, SVC_SECURITY},
        {"Desk officer", "the Secret Intelligence Service", 70, 93, SVC_ADVERSARY},
        {"Second secretary (a station officer)", "the United States Embassy", 65, 90, SVC_ADVERSARY},
        {"Attaché (an intelligence officer)", "a West European embassy", 45, 75, SVC_THIRD}};
    return t;
}

// ============================================================================ VIENNA 1973-1977
static Theatre vienna() {
    Theatre t{};
    t.city = "Vienna"; t.country = "Austria";
    t.cover = "Second Secretary, Permanent Mission to the International Organisations"; t.embassy = "Embassy of the USSR, Reisnerstrasse 45-47, Vienna III";
    t.adversary = "the State Police and the Americans"; t.watchers = "Stapo"; t.currency = "dollars"; t.chief = "Rezident";
    t.lang = 1;
    t.sy = 1973; t.sm = 9; t.sd = 3; t.ey = 1977; t.em = 6; t.ed = 30;
    t.role = 1; t.subs = 1; t.colleagues = 6;
    t.price = 2; t.a4Base = 0.6f; t.heat0 = 10; t.blownHeat = 5; t.dangle = 0.15f; t.walkIn = 0.30f; t.valuePar = 1700;
    fill(t.district, {"Landstraße", "Innere Stadt", "Wieden", "Leopoldstadt", "Döbling", "Hietzing", "Donaustadt", "Klosterneuburg"});
    fill(t.dist, {0, 1, 1, 1, 2, 2, 3, 3});
    fill(t.density, {0.22f, 0.22f, 0.10f, 0.09f, 0.07f, 0.06f, 0.04f, 0.03f});
    streets(t, {"Ungargasse", "Rennweg", "Landstraßer Hauptstraße", "Wollzeile", "Bäckerstraße", "Naglergasse",
                "Favoritenstraße", "Wiedner Hauptstraße", "Margaretenstraße", "Praterstraße", "Taborstraße", "Obere Augartenstraße",
                "Billrothstraße", "Grinzinger Allee", "Sieveringer Straße", "Hietzinger Hauptstraße", "Lainzer Straße", "Maxingstraße",
                "Wagramer Straße", "Erzherzog-Karl-Straße", "Kagraner Platz", "Stadtplatz", "Weidlinger Straße", "Albrechtstraße"});
    fill(t.park, {"the Belvedere gardens", "the Stadtpark", "the Resselpark", "the Prater", "the Wertheimsteinpark", "the Schönbrunn park", "the Donaupark", "the Danube meadows"});
    fill(t.station, {"the Landstraße Stadtbahn station", "the Stephansplatz tram stop", "the Karlsplatz Stadtbahn station", "Praterstern station",
                     "the Nussdorfer Straße Stadtbahn station", "the Hietzing Stadtbahn station", "the Kagran tram terminus", "Klosterneuburg-Weidling station"});
    t.venue = {"Café Altmann", "Gasthaus zur Linde", "Café Kirchner", "Weinstube zum Schwarzen Adler", "Café Hirsch", "Gasthaus zum Goldenen Stern",
               "Café Edelweiss", "Heuriger zum Alten Presshaus", "Gasthaus zur Post", "Café Dreiklang"};
    t.meetT = {"%P, %S", "Konditorei, %S", "wine cellar, %S", "cinema foyer, %S", "hotel café, %S", "bench walk, %K"};
    t.dropT = {"loose stone, cemetery wall off %S", "hollow behind the fourth bench, %K", "behind the radiator, stairwell by %T",
               "magnetic container under the tram-stop rail, %S", "cellar grating, courtyard off %S", "under the marked root, footpath in %K"};
    t.signalT = {"chalk mark, advertising column, %S", "drawing-pin, bench in %K", "strip of tape, railing outside %T", "chalk mark, hydrant, %S"};
    t.male = {"Franz", "Karl", "Josef", "Heinz", "Walter", "Herbert", "Gerhard", "Helmut", "Peter", "Wolfgang", "Kurt", "Erich", "Otto",
              "Rudolf", "Friedrich", "Anton", "Leopold", "Ernst", "Alfred", "Johann", "Georg", "Manfred", "Günther", "Robert", "Thomas",
              "James", "Henrik", "Lars", "Paul", "Viktor"};
    t.female = {"Maria", "Anna", "Elisabeth", "Helga", "Ingrid", "Christine", "Brigitte", "Renate", "Monika", "Gertrude"};
    t.last = {"Gruber", "Huber", "Bauer", "Wagner", "Pichler", "Steiner", "Moser", "Mayer", "Hofer", "Leitner", "Berger", "Fuchs", "Eder",
              "Fischer", "Schmid", "Winkler", "Weber", "Schwarz", "Maier", "Reiter", "Brunner", "Lang", "Baumgartner", "Auer", "Wolf",
              "Aigner", "Wallner", "Haas", "Koller", "Lehner", "Binder", "Kaiser", "Strobl", "Holzer", "Lindqvist", "Harrington",
              "Whitcombe", "Van Leeuwen", "Kessler", "Novotny"};
    t.jobs[POLITICAL] = {
        {"Conference officer", "the MBFR delegation of a NATO state", 55, 82, 62, 10, false},
        {"Second secretary", "the embassy of a NATO state", 50, 78, 60, 20, false},
        {"Press officer", "the OPEC secretariat", 40, 65, 25, 55, false},
        {"Foreign-affairs correspondent", "a Vienna daily", 25, 48, 8, 85, false},
        {"Section head", "the Austrian Foreign Ministry", 45, 70, 40, 25, false},
        {"Secretary to the ambassador", "a Western embassy", 55, 85, 58, 5, false},
        {"Parliamentary aide", "the Socialist Party club in the Nationalrat", 30, 55, 12, 60, false},
        {"Stringer", "a Western news agency", 20, 45, 6, 80, false}};
    t.jobs[MILITARY] = {
        {"Military adviser", "the MBFR delegation of a NATO state", 60, 88, 70, 5, true},
        {"Clerk", "the defence attaché's office of a NATO embassy", 50, 80, 66, 5, false},
        {"Staff officer", "the Austrian Army intelligence office", 45, 70, 50, 5, true},
        {"Courier sergeant", "a NATO forces liaison detachment", 45, 75, 60, 5, true},
        {"Analyst", "the military section of a Western embassy", 50, 78, 62, 10, false},
        {"Arms salesman", "a Styrian weapons works", 30, 55, 20, 15, true}};
    t.jobs[SCITECH] = {
        {"Safeguards inspector", "the International Atomic Energy Agency", 55, 85, 45, 15, false},
        {"Physicist", "the IAEA laboratory at Seibersdorf", 45, 75, 35, 15, false},
        {"Industrial development officer", "UNIDO", 25, 50, 15, 20, false},
        {"Sales manager", "an Austrian electronics trading firm", 45, 78, 20, 10, false},
        {"Metallurgist", "a Linz steel works", 35, 62, 30, 5, true},
        {"Systems analyst", "the institute for systems analysis at Laxenburg", 40, 68, 15, 25, false},
        {"Export clerk", "a firm of forwarding agents", 40, 70, 12, 5, false}};
    t.jobs[SECURITY] = {
        {"Inspector", "the State Police", 55, 82, 60, 5, true},
        {"Passport officer", "a Western consulate", 40, 65, 55, 5, false},
        {"Clerk", "the aliens' section, Interior Ministry", 40, 68, 35, 5, false},
        {"Local security guard", "the United States Embassy", 45, 72, 58, 5, true},
        {"Interpreter", "the refugee reception camp at Traiskirchen", 35, 60, 30, 10, false},
        {"Private detective", "self-employed (ex-gendarmerie)", 25, 50, 5, 15, true}};
    t.reqs = {
        {MILITARY, "MBFR: Western positions", "Negotiating instructions, fall-back positions and internal differences of the NATO delegations at the force-reduction talks.", 0, 240, 0, RK_COLLECT},
        {SCITECH, "IAEA safeguards", "Inspection reports, safeguards agreements and data on the nuclear programmes of third countries held by the Agency.", 0, 200, 0, RK_COLLECT},
        {POLITICAL, "OPEC price decisions", "Advance knowledge of OPEC ministerial decisions on prices and production; positions of the Arab delegations.", 1, 130, 150, RK_COLLECT},
        {SECURITY, "The American station", "Identification of officers of the main adversary's station in Vienna, their cover posts, vehicles and methods against Soviet citizens.", 0, 160, 0, RK_COLLECT},
        {SCITECH, "Embargoed electronics", "Channels for acquiring integrated circuits and computer equipment on the CoCom lists through Austrian trading and forwarding firms.", 2, 220, 0, RK_COLLECT},
        {POLITICAL, "Service A: Radio Free Europe", "ACTIVE MEASURE. Place material in the Austrian and West German press on the financing and staffing of the emigre radio stations in Munich. Three placements required.", 3, 3, 300, RK_ACTIVE},
        {POLITICAL, "Austrian Middle East policy", "Intentions of the Chancellor's office in its Middle East initiatives; contacts with the Arab side.", 4, 120, 180, RK_COLLECT},
        {SECURITY, "State Police methods", "Organisation, radio procedure and working methods of the State Police surveillance groups.", 5, 130, 0, RK_COLLECT},
        {POLITICAL, "European security conference: Basket III", "Western tactics on human contacts and information at the Geneva stage and before the final act.", 8, 150, 240, RK_COLLECT},
        {MILITARY, "NATO forces in the Central Region", "Data tabled or withheld by the West on its own force levels; the real figures behind them.", 10, 200, 0, RK_COLLECT},
        {SCITECH, "Uranium enrichment in third countries", "Agency knowledge of enrichment and reprocessing work in the FRG, Japan, South Africa, Israel and Pakistan.", 12, 220, 0, RK_COLLECT},
        {POLITICAL, "The Chinese mission", "Activities, contacts and reporting of the Chinese embassy and its approaches to the East European missions.", 14, 120, 0, RK_COLLECT},
        {SECURITY, "Emigre organisations", "Staff, funding and Western sponsors of the emigre centres working from Austrian territory.", 16, 130, 0, RK_COLLECT},
        {POLITICAL, "Service A: neutron weapons", "ACTIVE MEASURE. Place material in the European press on American plans for enhanced-radiation warheads in Europe. Three placements required.", 40, 3, 240, RK_ACTIVE},
        {SCITECH, "Numerically controlled machine tools", "Documentation and samples of control units for multi-axis machine tools; firms willing to supply.", 20, 200, 0, RK_COLLECT},
        {MILITARY, "US Army Europe signals", "Cipher procedure, call-signs and message handling of American forces in Europe, through personnel passing through Vienna.", 22, 240, 0, RK_COLLECT},
        {POLITICAL, "Western reaction to Helsinki", "Assessments in the Western embassies of how the Final Act is to be used against the socialist states.", 26, 140, 200, RK_COLLECT},
        {POLITICAL, "The new American administration", "Reporting of the Western embassies on the foreign-policy intentions of the incoming administration in Washington.", 38, 140, 150, RK_COLLECT}};
    t.hist = {
        {1973, 9, 28, "Embassy bulletin", "Hostages at Marchegg", "Armed men have seized hostages from a train carrying emigrants at the frontier station of Marchegg. The gendarmerie and State Police are fully occupied and will remain so for days.", 0, -0.25f, 0, H_NONE},
        {1973, 10, 17, "Embassy bulletin", "The oil embargo", "The Arab oil producers have announced production cuts and an embargo. Everything the OPEC secretariat knows is now worth having, and it sits a tram-ride from this Embassy.", 0, 0, 0, H_NONE},
        {1973, 10, 30, "Embassy bulletin", "Force-reduction talks open", "The talks on mutual force reductions in Central Europe open in Vienna today. Every NATO capital has sent a delegation, with its clerks, its interpreters and its military advisers. They will be here for years.", 0.05f, 0, 0, H_NONE},
        {1974, 4, 25, "Line KR", "Arrest in Bonn", "A personal assistant to the Federal Chancellor has been arrested as an agent of the friends in Berlin. Expect every Western security service to look again at officials with Eastern connections. Agents in government posts are to be handled with particular care.", 0.05f, 0, 0, H_NONE},
        {1975, 8, 1, "Embassy bulletin", "Helsinki Final Act", "The Final Act of the Conference on Security and Co-operation in Europe was signed today. Centre requires Western assessments of how the humanitarian provisions will be used.", 0, 0, 0, H_NONE},
        {1975, 12, 21, "Embassy bulletin", "Attack on the OPEC building", "An armed group has stormed the OPEC ministers' meeting on the Ring and taken the ministers hostage. Vienna is full of police; checkpoints on the roads out. For some weeks the city will be watched as it has never been.", 0.15f, 0.7f, 0, H_NONE},
        {1976, 9, 9, "Embassy bulletin", "Death of Mao", "Mao Tse-tung is dead. Centre requires everything obtainable on the succession from the Chinese mission and from those who talk to it.", 0, 0, 0, H_NONE},
        {1977, 1, 20, "Embassy bulletin", "New administration in Washington", "The new American President has taken office and speaks of human rights. The Western missions here are waiting for instructions; so are we.", 0, 0, 0, H_NONE}};
    t.local = "Vienna is neutral ground. The State Police are few and mostly watch the Arabs. Do not let that make you careless: the Americans run their own people in these streets, and every second man who is friendly to you at an Agency reception reports to somebody.";
    t.tour = "CENTRE, 1972-1973\n\nYou spent twenty months at Yasenevo: a desk in the Third Department, reading other officers' cables from Britain and Scandinavia, and a course at the Red Banner Institute. You learned how a cable from the field looks from the receiving end - how little of it is read, and by whom.\n\nYour London file was well regarded. In the summer of 1973 the personnel directorate offered you Vienna - the best opening a major can hope for.";
    t.slot = 1; t.cityRu = "ВЕНА";
    fill(t.at, {Pt{0.60f, 0.60f}, Pt{0.50f, 0.50f}, Pt{0.47f, 0.67f}, Pt{0.62f, 0.41f}, Pt{0.40f, 0.23f}, Pt{0.20f, 0.68f}, Pt{0.85f, 0.33f}, Pt{0.21f, 0.11f}});
    t.river = {{0.26f, 0.0f}, {0.40f, 0.12f}, {0.52f, 0.22f}, {0.66f, 0.32f}, {0.78f, 0.46f}, {0.90f, 0.56f}, {1.0f, 0.62f}};
    t.spies = {
        {"Inspector, counter-espionage group", "the State Police", 62, 88, SVC_SECURITY},
        {"Observer, surveillance group", "the State Police", 45, 68, SVC_SECURITY},
        {"Second secretary (a station officer)", "the United States Embassy", 70, 93, SVC_ADVERSARY},
        {"Vice-consul (a station officer)", "the British Embassy", 65, 88, SVC_ADVERSARY},
        {"Press attaché (an officer of the Federal Intelligence Service)", "the West German Embassy", 60, 85, SVC_THIRD},
        {"Correspondent (an officer of a Middle Eastern service)", "a foreign news agency", 45, 72, SVC_THIRD}};
    return t;
}

// ============================================================================ WASHINGTON 1979-1984
static Theatre washington() {
    Theatre t{};
    t.city = "Washington"; t.country = "the United States";
    t.cover = "First Secretary (Political)"; t.embassy = "Embassy of the USSR, 1125 16th Street NW, Washington DC";
    t.adversary = "the FBI"; t.watchers = "FBI"; t.currency = "dollars"; t.chief = "Rezident";
    t.lang = 0;
    t.sy = 1979; t.sm = 10; t.sd = 1; t.ey = 1984; t.em = 6; t.ed = 29;
    t.role = 2; t.subs = 3; t.colleagues = 6;
    t.price = 3; t.a4Base = 1.2f; t.heat0 = 26; t.blownHeat = 10; t.dangle = 0.15f; t.walkIn = 0.50f; t.valuePar = 2300;
    fill(t.district, {"Downtown", "Dupont Circle", "Georgetown", "Capitol Hill", "Arlington", "Bethesda", "Alexandria", "Silver Spring"});
    fill(t.dist, {0, 1, 1, 1, 2, 2, 3, 3});
    fill(t.density, {0.40f, 0.30f, 0.24f, 0.24f, 0.18f, 0.14f, 0.11f, 0.10f});
    streets(t, {"K Street", "L Street", "Vermont Avenue", "Connecticut Avenue", "P Street", "New Hampshire Avenue",
                "M Street", "Wisconsin Avenue", "O Street", "Pennsylvania Avenue SE", "East Capitol Street", "8th Street SE",
                "Wilson Boulevard", "Lee Highway", "Clarendon Boulevard", "Old Georgetown Road", "Woodmont Avenue", "River Road",
                "King Street", "Duke Street", "Mount Vernon Avenue", "Georgia Avenue", "Colesville Road", "Sligo Avenue"});
    fill(t.park, {"Franklin Square", "Rock Creek Park", "the canal towpath", "Lincoln Park", "Glencarlyn Park", "Cabin John park", "Fort Ward Park", "Sligo Creek Park"});
    fill(t.station, {"the Farragut North metro entrance", "the Dupont Circle metro entrance", "the Foggy Bottom metro entrance", "the Capitol South metro entrance",
                     "the Rosslyn metro entrance", "the Bethesda bus terminal", "Alexandria Union Station", "the Silver Spring metro entrance"});
    t.venue = {"the Hilltop Diner", "Murphy's Tavern", "the Blue Parrot Lounge", "Rosie's Coffee Shop", "the Old Dominion Grill", "the Anchor Bar",
               "Sal's Pizzeria", "the Red Fox Inn", "the Lamplighter", "the Whistle Stop"};
    t.meetT = {"%P, %S", "shopping-mall cafeteria, %S", "hotel cocktail lounge, %S", "bowling alley, %S", "pizza parlour, %S", "picnic tables, %K"};
    t.dropT = {"base of the utility pole, %S", "under the footbridge, %K", "behind the telephone booth by %T", "magnetic box under the guard rail, %S",
               "hollow log off the trail, %K", "under the corner mailbox, %S"};
    t.signalT = {"chalk line, mailbox, %S", "strip of tape, signpost by %T", "thumbtack, utility pole, %S", "soda can at the kerb, %S"};
    t.male = {"James", "Robert", "John", "Michael", "William", "David", "Richard", "Charles", "Thomas", "Gary", "Ronald", "Donald", "Kenneth",
              "Steven", "Larry", "Dennis", "Jerry", "Frank", "Raymond", "Douglas", "Roger", "Gerald", "Walter", "Harold", "Carl", "Arthur",
              "Wayne", "Bruce", "Glenn", "Dale"};
    t.female = {"Mary", "Linda", "Barbara", "Patricia", "Susan", "Nancy", "Karen", "Carol", "Sharon", "Donna"};
    t.last = {"Anderson", "Baker", "Carlson", "Dawson", "Ellis", "Foster", "Garrett", "Hayes", "Ingram", "Jensen", "Keller", "Lawson", "Mercer",
              "Nolan", "Osborne", "Pratt", "Quinn", "Reeves", "Sutton", "Tucker", "Underwood", "Vance", "Whitaker", "Yates", "Brennan",
              "Calloway", "Dalton", "Emerson", "Fleming", "Granger", "Holloway", "Jarvis", "Kendrick", "Lowell", "Mabry", "Norris", "Pruitt",
              "Ramsey", "Stanton", "Thornton"};
    t.jobs[POLITICAL] = {
        {"Staff assistant", "a Senate committee", 45, 75, 40, 40, false},
        {"Legislative aide", "a member of the House of Representatives", 30, 58, 15, 50, false},
        {"Desk officer", "the Department of State", 60, 85, 72, 15, false},
        {"Research analyst", "a Washington policy institute", 30, 55, 10, 70, false},
        {"Reporter", "a wire service", 25, 48, 6, 85, false},
        {"Secretary", "the National Security Council staff", 65, 92, 88, 5, false},
        {"Trade specialist", "the Department of Commerce", 40, 65, 45, 15, false},
        {"Consultant", "a K Street lobbying firm", 30, 55, 8, 60, false}};
    t.jobs[MILITARY] = {
        {"Communications watch officer", "the Navy's Atlantic submarine command", 70, 95, 80, 5, true},
        {"Budget analyst", "the Department of the Army", 50, 75, 62, 5, false},
        {"Message-centre clerk", "the Joint Chiefs of Staff", 62, 90, 82, 5, false},
        {"Systems engineer", "a Northern Virginia defense contractor", 55, 85, 55, 10, false},
        {"Analyst", "the Defense Intelligence Agency", 60, 88, 80, 5, false},
        {"Warrant officer", "an Army signals unit", 55, 82, 70, 5, true},
        {"Retired officer", "a defense consulting firm", 35, 60, 25, 30, true}};
    t.jobs[SCITECH] = {
        {"Engineer", "an aerospace contractor", 55, 85, 58, 10, false},
        {"Programmer", "the federal office of a supercomputer maker", 50, 80, 40, 10, false},
        {"Researcher", "the Naval Research Laboratory", 60, 88, 74, 15, false},
        {"Patent examiner", "the Patent Office", 30, 55, 20, 5, false},
        {"Sales engineer", "a semiconductor firm", 45, 75, 30, 10, false},
        {"Physicist", "the Washington office of a national laboratory", 60, 90, 80, 20, false},
        {"Librarian", "a technical reports service", 30, 55, 10, 5, false}};
    t.jobs[SECURITY] = {
        {"Communications specialist", "the National Security Agency", 70, 95, 88, 5, false},
        {"File clerk", "the FBI's Washington field office", 60, 88, 84, 5, false},
        {"Former trainee", "the Central Intelligence Agency", 50, 80, 30, 5, false},
        {"Detective", "the Metropolitan Police", 30, 55, 25, 10, true},
        {"Guard", "a firm of embassy security contractors", 30, 58, 20, 5, true},
        {"Private investigator", "self-employed (ex-Bureau)", 30, 55, 8, 15, true}};
    t.reqs = {
        {POLITICAL, "American reaction to Afghanistan", "Measures planned against the USSR; the grain embargo, the Olympic boycott, differences inside the administration and with the allies.", 2, 150, 150, RK_COLLECT},
        {MILITARY, "Fleet ballistic missile submarine communications", "Cipher material, key lists and operating procedure of the submarine broadcast; patrol areas.", 0, 300, 0, RK_COLLECT},
        {SCITECH, "Very-high-speed integrated circuits", "Design rules, process documentation and test results of the Pentagon microelectronics programme.", 0, 260, 0, RK_COLLECT},
        {SECURITY, "FBI surveillance of the Embassy", "Organisation, radio procedure, vehicles, fixed posts and technical means of the Bureau's Washington field office.", 0, 180, 0, RK_COLLECT},
        {MILITARY, "MX basing", "Decisions on the deployment mode of the new intercontinental missile; hardness, numbers, timetable.", 3, 220, 0, RK_COLLECT},
        {POLITICAL, "The transition", "Foreign and defence policy intentions and appointments of the incoming administration.", 12, 150, 120, RK_COLLECT},
        {POLITICAL, "Service A: the neutron bomb and Europe", "ACTIVE MEASURE. Place material with American journalists and research institutes on allied opposition to new nuclear weapons in Europe. Three placements required.", 6, 3, 300, RK_ACTIVE},
        {POLITICAL, "Poland", "American contingency planning, contacts with the Polish opposition, sanctions under consideration.", 11, 150, 180, RK_COLLECT},
        {SCITECH, "Low-observable aircraft", "Materials, shaping and test data of the 'stealth' programmes.", 10, 300, 0, RK_COLLECT},
        {SECURITY, "The main adversary's personnel", "Identities and cover of officers of the Central Intelligence Agency under training or assignment to Moscow.", 8, 220, 0, RK_COLLECT},
        {MILITARY, "RYAN: military indicators", "Indicators of preparation for a surprise nuclear missile attack: alert states, command post activity, movement of senior officers.", 20, 200, 0, RK_COLLECT},
        {POLITICAL, "RYAN: political indicators", "Indicators of preparation for a surprise nuclear missile attack: late working at the White House and State Department, civil defence, blood stocks.", 20, 160, 0, RK_COLLECT},
        {SCITECH, "Space shuttle military payloads", "Payload manifests, reconnaissance and communications satellites to be carried, Vandenberg launch plans.", 18, 220, 0, RK_COLLECT},
        {MILITARY, "Pershing II and cruise missile deployment", "Timetable, basing, readiness and release procedure of the new missiles for Europe.", 30, 220, 240, RK_COLLECT},
        {SCITECH, "Strategic Defense Initiative", "Organisation, budget, contractors and technical direction of the space-based missile defence programme.", 42, 300, 0, RK_COLLECT},
        {SECURITY, "NSA collection against Soviet communications", "What the adversary reads: programmes against Soviet military and diplomatic communications and undersea cables.", 14, 260, 0, RK_COLLECT},
        {POLITICAL, "Arms-control positions", "Instructions to the American delegations at the Geneva talks on strategic and intermediate-range forces.", 26, 220, 0, RK_COLLECT},
        {POLITICAL, "Service A: militarisation of space", "ACTIVE MEASURE. Place material with American scientists' organisations and the press on the cost and unworkability of space weapons. Three placements required.", 44, 3, 300, RK_ACTIVE},
        {SCITECH, "Supercomputers", "Architecture, software and export-control procedures for the newest vector machines.", 28, 240, 0, RK_COLLECT},
        {MILITARY, "Naval nuclear propulsion", "Quieting measures and reactor data of the newest attack submarines.", 34, 280, 0, RK_COLLECT}};
    t.hist = {
        {1979, 11, 4, "Embassy bulletin", "Hostages in Tehran", "The American Embassy in Tehran has been seized with its staff. The administration can think of nothing else. For the moment, neither can the Bureau.", 0, -0.15f, 0, H_NONE},
        {1979, 12, 27, "Rezident", "Afghanistan", "A limited contingent of Soviet forces has entered Afghanistan at the request of its government. Expect the worst atmosphere in this city since 1962: restrictions on our travel, hostile press, and every agent's nerves tested. Centre wants American intentions at once.", 0.1f, 0, 4, H_NONE},
        {1980, 11, 4, "Embassy bulletin", "Presidential election", "The Republican candidate has won by a wide margin. Centre requires his appointments and intentions before he takes office.", 0, 0, 0, H_NONE},
        {1981, 3, 30, "Embassy bulletin", "President shot", "The President has been shot and wounded outside a Washington hotel. The city is full of police and federal agents; operational acts in the centre are out of the question for some days.", 0, 0.6f, 0, H_NONE},
        {1981, 5, 25, "Centre", "Operation RYAN", "", 0, 0, 0, H_RYAN},
        {1981, 12, 13, "Embassy bulletin", "Martial law in Poland", "Martial law has been declared in Poland. American sanctions against the USSR are to be expected.", 0.05f, 0, 0, H_NONE},
        {1982, 11, 10, "Centre", "Death of L. I. Brezhnev", "The General Secretary is dead. Comrade Yu. V. Andropov, for fifteen years Chairman of this Committee, succeeds him. You may assume that reporting from this rezidentura will now be read at the very top, and read by a professional.", 0, 0, 0, H_NONE},
        {1983, 3, 23, "Embassy bulletin", "The 'Star Wars' speech", "The President has announced a programme to build a defence against ballistic missiles based in space. Centre's requirements will follow within days and will be of the first priority.", 0, 0, 0, H_NONE},
        {1983, 9, 1, "Rezident", "The Korean airliner", "A South Korean airliner has been destroyed in Soviet airspace off Sakhalin. Demonstrations outside the Embassy; Aeroflot landing rights withdrawn. The Bureau will use the mood. Reduce operational acts to those that cannot wait.", 0.15f, 0.3f, 4, H_NONE},
        {1983, 11, 7, "Centre", "FLASH - RYAN", "", 0, 0, 0, H_ABLE},
        {1984, 2, 9, "Centre", "Death of Yu. V. Andropov", "The General Secretary is dead. Work continues under the existing instructions.", 0, 0, 0, H_NONE}};
    t.local = "This is the capital of the main adversary. The Bureau has more watchers than we have officers, radio cars on every avenue, and a lookout post opposite our gate. You may not travel more than twenty-five miles from the White House without notifying the State Department. Assume coverage until you have proved otherwise, and then go on assuming it.";
    t.tour = "CENTRE, 1977-1979\n\nTwo years in the First Department - North America - as a desk officer for the Washington rezidentura: you drafted the requirements that other men ran risks to meet, and graded what they sent. You also sat on two commissions of inquiry into failed operations, and saw how blame is distributed.\n\nIn 1979 the department's chief put your name forward for Washington, as head of a line.";
    t.slot = 2; t.cityRu = "ВАШИНГТОН";
    fill(t.at, {Pt{0.52f, 0.46f}, Pt{0.47f, 0.35f}, Pt{0.36f, 0.41f}, Pt{0.66f, 0.52f}, Pt{0.28f, 0.59f}, Pt{0.30f, 0.14f}, Pt{0.45f, 0.88f}, Pt{0.59f, 0.09f}});
    t.river = {{0.02f, 0.18f}, {0.16f, 0.30f}, {0.28f, 0.42f}, {0.38f, 0.52f}, {0.46f, 0.62f}, {0.52f, 0.74f}, {0.55f, 0.86f}, {0.60f, 1.0f}};
    t.spies = {
        {"Special agent, Soviet counter-intelligence squad", "the FBI's Washington field office", 75, 96, SVC_SECURITY},
        {"Surveillance specialist", "the FBI's Washington field office", 48, 72, SVC_SECURITY},
        {"Case officer, Soviet division", "the Central Intelligence Agency", 78, 97, SVC_ADVERSARY},
        {"Analyst, counter-intelligence staff", "the Central Intelligence Agency", 70, 93, SVC_ADVERSARY},
        {"Investigator", "the Naval Investigative Service", 55, 80, SVC_SECURITY},
        {"Attaché (an intelligence officer)", "the embassy of a NATO ally", 45, 75, SVC_THIRD}};
    return t;
}

// ============================================================================ BONN 1986-1991
static Theatre bonn() {
    Theatre t{};
    t.city = "Bonn"; t.country = "the Federal Republic of Germany";
    t.cover = "Counsellor"; t.embassy = "Embassy of the USSR, Waldstrasse 42, Bonn-Bad Godesberg";
    t.adversary = "the Office for the Protection of the Constitution"; t.watchers = "BfV"; t.currency = "marks"; t.chief = "Head of the Fourth Department";
    t.lang = 1;
    t.sy = 1986; t.sm = 9; t.sd = 1; t.ey = 1991; t.em = 6; t.ed = 28;
    t.role = 3; t.subs = 5; t.colleagues = 3;
    t.price = 4; t.a4Base = 0.9f; t.heat0 = 20; t.blownHeat = 8; t.dangle = 0.11f; t.walkIn = 0.35f; t.valuePar = 4200;
    fill(t.district, {"Bad Godesberg", "Bonn-Zentrum", "Regierungsviertel", "Beuel", "Königswinter", "Siegburg", "Köln", "Düsseldorf"});
    fill(t.dist, {0, 1, 1, 1, 2, 2, 3, 3});
    fill(t.density, {0.28f, 0.18f, 0.26f, 0.09f, 0.06f, 0.06f, 0.14f, 0.07f});
    streets(t, {"Koblenzer Straße", "Rheinallee", "Plittersdorfer Straße", "Sternstraße", "Poppelsdorfer Allee", "Kaiserstraße",
                "Adenauerallee", "Heussallee", "Dahlmannstraße", "Friedrich-Breuer-Straße", "Rheinaustraße", "Siegburger Straße",
                "Hauptstraße", "Drachenfelsstraße", "Bahnhofstraße", "Holzgasse", "Mühlenstraße", "Wilhelmstraße",
                "Aachener Straße", "Severinstraße", "Zülpicher Straße", "Bilker Allee", "Oststraße", "Nordstraße"});
    fill(t.park, {"the Kurpark", "the Hofgarten", "the Rheinaue park", "the Beuel riverbank", "the Nachtigallental", "the Michaelsberg slopes", "the Stadtwald", "the Volksgarten"});
    fill(t.station, {"Bad Godesberg station", "Bonn Hauptbahnhof", "the Heussallee U-Bahn entrance", "Beuel station", "the Königswinter ferry landing",
                     "Siegburg station", "Köln Hauptbahnhof", "Düsseldorf Hauptbahnhof"});
    t.venue = {"Gasthaus zum Löwen", "Weinhaus Jakobs", "Zur Alten Post", "the Ratskeller", "Café Kleimann", "Weinstube Rebstock", "Gasthof Adler",
               "Bierhaus am Rhein", "Café Rheinblick", "Eiscafé Venezia"};
    t.meetT = {"%P, %S", "Konditorei, %S", "department-store restaurant, %S", "station buffet at %T", "hotel bar, %S", "bench walk, %K"};
    t.dropT = {"loose stone, vineyard wall off %S", "hollow behind the waste bin, %K", "behind the timetable case, %T", "magnetic container under the crash barrier, %S",
               "drain cover, car park off %S", "under the marked boulder, forest path in %K"};
    t.signalT = {"chalk mark, advertising column, %S", "drawing-pin, bench in %K", "strip of tape, bicycle stand at %T", "chalk mark, electricity box, %S"};
    t.male = {"Hans", "Klaus", "Jürgen", "Dieter", "Wolfgang", "Manfred", "Günter", "Horst", "Uwe", "Bernd", "Rainer", "Helmut", "Werner", "Peter",
              "Michael", "Thomas", "Andreas", "Stefan", "Frank", "Joachim", "Rolf", "Detlef", "Norbert", "Volker", "Hartmut", "Reinhard",
              "Ulrich", "Jörg", "Matthias", "Holger"};
    t.female = {"Ursula", "Renate", "Monika", "Karin", "Petra", "Sabine", "Gabriele", "Brigitte", "Angelika", "Heike"};
    t.last = {"Müller", "Schmidt", "Schneider", "Fischer", "Weber", "Meyer", "Wagner", "Becker", "Schulz", "Hoffmann", "Schäfer", "Koch", "Bauer",
              "Richter", "Klein", "Wolf", "Schröder", "Neumann", "Schwarz", "Zimmermann", "Braun", "Krüger", "Hofmann", "Hartmann", "Lange",
              "Schmitt", "Werner", "Krause", "Meier", "Lehmann", "Köhler", "Herrmann", "König", "Walter", "Kaiser", "Fuchs", "Peters", "Lang",
              "Scholz", "Möller"};
    t.jobs[POLITICAL] = {
        {"Desk officer", "the Foreign Office", 60, 85, 70, 15, false},
        {"Secretary", "the Federal Chancellery", 65, 92, 80, 5, false},
        {"Research assistant", "a parliamentary group in the Bundestag", 35, 62, 15, 55, false},
        {"Correspondent", "the Bonn press corps", 25, 50, 6, 85, false},
        {"Official", "the Ministry for Inner-German Relations", 55, 82, 62, 15, false},
        {"Programme officer", "a party political foundation", 30, 55, 10, 60, false},
        {"Parliamentary researcher", "the Greens", 25, 50, 5, 65, false},
        {"Economist", "the Federal Ministry of Economics", 50, 75, 55, 20, false}};
    t.jobs[MILITARY] = {
        {"Staff officer", "the Federal Ministry of Defence", 60, 88, 76, 5, true},
        {"Clerk", "the Federal Office for Defence Procurement, Koblenz", 55, 82, 62, 5, false},
        {"Sergeant-major", "a NATO signals liaison unit", 55, 85, 70, 5, true},
        {"Engineer", "the Bonn office of an armoured-vehicle maker", 50, 78, 50, 10, false},
        {"Interpreter", "a NATO headquarters staff", 50, 80, 66, 10, false},
        {"Secretary", "the defence attaché of a NATO embassy", 55, 85, 62, 5, false},
        {"Defence correspondent", "a Hamburg weekly", 25, 48, 8, 80, false}};
    t.jobs[SCITECH] = {
        {"Engineer", "an aerospace concern", 55, 85, 58, 10, false},
        {"Chemist", "a Leverkusen chemicals concern", 40, 68, 40, 10, false},
        {"Export manager", "a machine-tool firm", 50, 82, 22, 10, false},
        {"Computer engineer", "the Bonn office of a Munich electronics concern", 50, 80, 45, 10, false},
        {"Researcher", "the nuclear research centre at Jülich", 55, 85, 60, 20, false},
        {"Freelance programmer", "with accounts on the research networks", 40, 78, 5, 10, false},
        {"Patent lawyer's assistant", "a Düsseldorf practice", 30, 55, 10, 5, false}};
    t.jobs[SECURITY] = {
        {"Clerk", "the Office for the Protection of the Constitution, Köln", 62, 90, 84, 5, false},
        {"Officer", "the Federal Border Guard", 35, 60, 45, 5, true},
        {"Cipher clerk", "a NATO embassy", 65, 92, 78, 5, false},
        {"Official", "the Bonn liaison office of the Federal Intelligence Service", 60, 88, 82, 5, false},
        {"Detective", "the state criminal police, Düsseldorf", 35, 60, 35, 10, true},
        {"Security consultant", "self-employed (ex-police)", 25, 50, 5, 15, true}};
    t.reqs = {
        {POLITICAL, "The Federal Government and the INF talks", "Bonn's real position on the 'double zero'; the Pershing IA question; differences between the Chancellery and the Foreign Office.", 0, 170, 240, RK_COLLECT},
        {SCITECH, "Embargoed machine tools and controls", "Sources of multi-axis machine tools, numerical controls and software on the CoCom lists; firms and intermediaries willing to supply.", 0, 280, 0, RK_COLLECT},
        {MILITARY, "NATO command-post exercises", "Scenario documents, nuclear release procedure and after-action reports of the biennial alliance exercises.", 0, 260, 0, RK_COLLECT},
        {SECURITY, "Counter-intelligence in the Federal Republic", "Working methods, current cases and sources of the Cologne office directed against the rezidentura and the friends.", 0, 200, 0, RK_COLLECT},
        {POLITICAL, "Service A: modernisation of short-range missiles", "ACTIVE MEASURE. Place material in the West German press and with the parliamentary opposition against a successor to the Lance missile. Three placements required.", 8, 3, 300, RK_ACTIVE},
        {POLITICAL, "Bonn's policy toward the GDR", "Intentions for the visit of the General Secretary of the SED; credits, transit, the citizenship question.", 6, 150, 180, RK_COLLECT},
        {SCITECH, "Research networks", "Access to computers of research establishments and contractors in the West through the open data networks; source code, designs, passwords.", 5, 240, 0, RK_COLLECT},
        {MILITARY, "Combat aircraft and armour upgrades", "Development status and performance data of the multi-role aircraft and the improved main battle tank.", 10, 240, 0, RK_COLLECT},
        {SCITECH, "West German firms in the American space-defence programme", "Contracts, technologies and personnel of firms in the Federal Republic working for the SDI organisation.", 12, 220, 0, RK_COLLECT},
        {POLITICAL, "West German assessments of the Soviet reforms", "How the Chancellery, the Foreign Office and industry judge perestroika and its chances; credits under consideration.", 22, 170, 0, RK_COLLECT},
        {SECURITY, "The Federal Intelligence Service", "Personnel under cover, sources and reporting on the USSR and the GDR.", 16, 240, 0, RK_COLLECT},
        {POLITICAL, "Service A: chemical weapons depots", "ACTIVE MEASURE. Place material in the regional press on American chemical munitions stored in the Palatinate. Three placements required.", 26, 3, 300, RK_ACTIVE},
        {MILITARY, "Bundeswehr structure for the 1990s", "Planning documents for force structure, conscription and mobilisation under falling numbers.", 28, 220, 0, RK_COLLECT},
        {POLITICAL, "The German question", "The Federal Government's real intentions toward the GDR after the opening of the frontier: timetable, money, the alliances.", 38, 260, 240, RK_COLLECT},
        {POLITICAL, "The 'two plus four' negotiations", "Bonn's negotiating positions and fall-back lines on alliance membership, force ceilings and the withdrawal of Soviet forces; what it will pay.", 42, 300, 180, RK_COLLECT},
        {SCITECH, "Semiconductor production equipment", "Lithography, clean-room and test equipment of the newest generation; documentation and channels.", 30, 260, 0, RK_COLLECT},
        {SECURITY, "The files of the friends", "What the West German services have obtained of the records of the Ministry for State Security, and whom they name.", 44, 220, 0, RK_COLLECT}};
    t.hist = {
        {1986, 10, 12, "Embassy bulletin", "Reykjavik", "The meeting between the General Secretary and the American President in Iceland ended without agreement, but everything is now on the table. Bonn's reactions are required.", 0, 0, 0, H_NONE},
        {1987, 5, 28, "Centre", "Incident on Red Square", "A West German amateur pilot has landed a light aircraft beside the Kremlin. The Minister of Defence has been dismissed. The departments here are looking for people to blame for many things; send nothing that is not solid.", 0, 0, 0, H_NONE},
        {1987, 9, 7, "Embassy bulletin", "The General Secretary of the SED in Bonn", "The East German leader begins an official visit to the Federal Republic today. The security services of the host are fully occupied with protection for the week.", 0, -0.3f, 0, H_NONE},
        {1987, 12, 8, "Embassy bulletin", "INF Treaty", "The treaty eliminating intermediate-range missiles was signed in Washington. Centre requires the Federal Government's intentions for what remains.", 0, 0, 0, H_NONE},
        {1989, 6, 12, "Embassy bulletin", "The General Secretary in Bonn", "M. S. Gorbachev begins his visit to the Federal Republic. Crowds in the Market Square. The whole West German security apparatus is on protective duty; and for once the population is pleased to see a Soviet official.", 0, -0.35f, -2, H_NONE},
        {1989, 11, 9, "Embassy bulletin", "The frontier is open", "", -0.1f, -0.2f, 0, H_WALL},
        {1989, 11, 28, "Embassy bulletin", "The Ten Points", "The Federal Chancellor has put a ten-point programme for German unity to the Bundestag without telling his allies or us. Centre requires, as the first priority of this rezidentura, to know how far and how fast he means to go.", 0, 0, 0, H_NONE},
        {1990, 1, 15, "Line KR", "The Normannenstrasse", "", 0.1f, 0, 0, H_ORPHANS},
        {1990, 7, 1, "Embassy bulletin", "Monetary union", "The West German mark is from today the currency of the GDR. The state we were allied with is ceasing to exist by the week.", 0, 0, 0, H_NONE},
        {1990, 10, 3, "Embassy bulletin", "Unification", "The GDR acceded to the Federal Republic at midnight. The friends' service no longer exists; its officers are being questioned, and some of them are answering. Every source ever shared with them, or known to them, must be treated as named.", 0.2f, 0, 4, H_NONE}};
    t.local = "You are the Rezident. The Cologne office is twenty minutes up the autobahn and has had forty years to study this Embassy. Your officers will take their habits from you. The friends in Berlin have more agents in this town than we do; do not rely on them, and do not let them know more of ours than they must.";
    t.tour = "CENTRE, 1984-1986\n\nYou served two years in Directorate K, external counter-intelligence, and they were the years to be there. In 1985 one agent after another inside the Committee and the military neighbours was uncovered and brought home; nobody at your level was told how. In the same summer the acting Rezident in London was recalled, questioned, and then vanished from under surveillance in Moscow - to reappear in Britain. You read the damage assessments. Every one of them was about who had known what.\n\nYour record abroad was clean and your tours had been good ones. In 1986 you were appointed Rezident in Bonn.";
    t.slot = 3; t.cityRu = "БОНН";
    fill(t.at, {Pt{0.49f, 0.67f}, Pt{0.41f, 0.42f}, Pt{0.45f, 0.54f}, Pt{0.59f, 0.40f}, Pt{0.69f, 0.75f}, Pt{0.77f, 0.28f}, Pt{0.25f, 0.17f}, Pt{0.09f, 0.06f}});
    t.river = {{0.74f, 1.0f}, {0.64f, 0.80f}, {0.57f, 0.64f}, {0.52f, 0.50f}, {0.49f, 0.38f}, {0.42f, 0.26f}, {0.32f, 0.14f}, {0.20f, 0.06f}, {0.14f, 0.0f}};
    t.spies = {
        {"Officer, counter-espionage department", "the Office for the Protection of the Constitution", 75, 95, SVC_SECURITY},
        {"Observer, surveillance group", "the Office for the Protection of the Constitution", 48, 70, SVC_SECURITY},
        {"Officer, Soviet desk", "the Federal Intelligence Service", 70, 93, SVC_ADVERSARY},
        {"Officer", "the Military Counter-Intelligence Service", 58, 85, SVC_SECURITY},
        {"Second secretary (a station officer)", "the United States Embassy", 70, 93, SVC_ADVERSARY},
        {"Attaché (an intelligence officer)", "the embassy of a NATO ally", 45, 75, SVC_THIRD}};
    return t;
}

// ============================================================================ MOSCOW, JUNE 1968 - THE TRAINING COURSE
// Not a posting: three weeks of practical exercises in the city before you are sent abroad. The "adversary"
// is the training brigade of the Seventh Directorate, and the people you meet are retired officers playing parts.
static Theatre moscow() {
    Theatre t{};
    t.slot = -1; t.training = true; t.cityRu = "МОСКВА";
    t.city = "Moscow"; t.country = "the exercise area";
    t.cover = "trainee, Red Banner Institute"; t.embassy = "School No. 101, in the woods beyond the ring road";
    t.adversary = "the training brigade of the Seventh Directorate"; t.watchers = "training brigade"; t.currency = "roubles"; t.chief = "Instructor";
    t.lang = 0;
    t.sy = 1968; t.sm = 6; t.sd = 3; t.ey = 1968; t.em = 6; t.ed = 30;
    t.role = 0; t.subs = 0; t.colleagues = 2;
    t.price = 1; t.a4Base = 0.8f; t.heat0 = 10; t.blownHeat = 0; t.dangle = 0; t.walkIn = 0; t.valuePar = 100;
    fill(t.district, {"Lubyanka", "Arbat", "Zamoskvorechye", "Krasnaya Presnya", "Sokolniki", "Lenin Hills", "Izmailovo", "Fili"});
    fill(t.dist, {0, 1, 1, 1, 2, 2, 3, 3});
    fill(t.density, {0.15f, 0.15f, 0.15f, 0.15f, 0.12f, 0.12f, 0.10f, 0.10f});
    fill(t.at, {Pt{0.54f, 0.42f}, Pt{0.40f, 0.48f}, Pt{0.56f, 0.64f}, Pt{0.34f, 0.34f}, Pt{0.72f, 0.22f}, Pt{0.28f, 0.76f}, Pt{0.88f, 0.38f}, Pt{0.13f, 0.50f}});
    t.river = {{0.0f, 0.40f}, {0.10f, 0.58f}, {0.22f, 0.66f}, {0.34f, 0.62f}, {0.44f, 0.56f}, {0.52f, 0.54f}, {0.62f, 0.58f}, {0.72f, 0.70f}, {0.84f, 0.80f}, {1.0f, 0.88f}};
    streets(t, {"Kuznetsky Most", "Sretenka", "Kirov Street", "the Arbat", "Sivtsev Vrazhek", "Gogol Boulevard",
                "Pyatnitskaya Street", "Bolshaya Ordynka", "Novokuznetskaya Street", "Bolshaya Gruzinskaya", "Malaya Bronnaya", "Krasnaya Presnya Street",
                "Rusakovskaya Street", "Stromynka", "Sokolnichesky Val", "Universitetsky Prospekt", "Michurinsky Prospekt", "Vorobyovskoye Shosse",
                "Pervomaiskaya Street", "Izmailovsky Boulevard", "Shchyolkovskoye Shosse", "Bolshaya Filyovskaya", "Kastanayevskaya Street", "Minskaya Street"});
    fill(t.park, {"the Alexander Garden", "the boulevard gardens", "the Tretyakov gardens", "the Zoo park", "Sokolniki Park", "the Lenin Hills embankment", "Izmailovo Park", "Fili Park"});
    fill(t.station, {"Dzerzhinskaya metro station", "Arbatskaya metro station", "Novokuznetskaya metro station", "Krasnopresnenskaya metro station",
                     "Sokolniki metro station", "Leninskiye Gory metro station", "Izmailovskaya metro station", "Fili metro station"});
    t.venue = {"the Pelmennaya", "Café Druzhba", "the Shashlychnaya", "Café Sever", "the Praga buffet", "the Blinnaya"};
    t.meetT = {"%P, %S", "tea-room, %S", "cinema foyer, %S", "bench walk, %K"};
    t.dropT = {"loose brick, courtyard wall off %S", "hollow behind the third bench, %K", "magnetic container under railing, %S", "drainpipe bracket, yard off %S"};
    t.signalT = {"chalk mark, lamp-post, %S", "drawing-pin, bench in %K", "strip of tape, railing outside %T"};
    t.male = {"Alan", "Brian", "Colin", "Derek", "Edward", "Frank", "Gordon", "Harold", "Kenneth", "Leonard", "Norman", "Raymond", "Stanley", "Walter"};
    t.female = {"Barbara", "Doreen", "Margaret", "Sheila"};
    t.last = {"Ashworth", "Barlow", "Chadwick", "Dacre", "Fenwick", "Garside", "Hartley", "Jessop", "Lomax", "Naylor", "Pickering", "Rudge", "Tetlow", "Varley"};
    for (int i = 0; i < 4; i++) t.jobs[i] = {{"'Clerk in a ministry' - a part played by a retired officer", "exercise FOREIGN CAPITAL", 50, 70, 50, 20, false}};
    t.local = "";
    t.tour = "";
    return t;
}

// ============================================================================ NEW DELHI 1973-1977
// The alternative to Vienna. Lightly watched, cheap, full of volunteers - and Centre has more Indian
// cabinet papers than it can read. The targets that count are the Americans, the Chinese and the bomb.
static Theatre delhi() {
    Theatre t{};
    t.slot = 1; t.cityRu = "ДЕЛИ"; t.gradeMult = 0.8f;
    t.city = "New Delhi"; t.country = "India";
    t.cover = "Second Secretary (Cultural)"; t.embassy = "Embassy of the USSR, Shantipath, Chanakyapuri, New Delhi";
    t.adversary = "the Intelligence Bureau and the Americans"; t.watchers = "Intelligence Bureau"; t.currency = "rupees"; t.chief = "Rezident";
    t.lang = 0;
    t.sy = 1973; t.sm = 9; t.sd = 3; t.ey = 1977; t.em = 6; t.ed = 30;
    t.role = 1; t.subs = 1; t.colleagues = 6;
    t.price = 6; t.a4Base = 0.45f; t.heat0 = 8; t.blownHeat = 2; t.dangle = 0.08f; t.walkIn = 0.55f; t.valuePar = 1500;
    fill(t.district, {"Chanakyapuri", "Connaught Place", "Raisina Hill", "Lodi Colony", "Old Delhi", "Defence Colony", "Hauz Khas", "Mehrauli"});
    fill(t.dist, {0, 1, 1, 1, 2, 2, 3, 3});
    fill(t.density, {0.20f, 0.14f, 0.20f, 0.07f, 0.05f, 0.05f, 0.03f, 0.02f});
    fill(t.at, {Pt{0.34f, 0.56f}, Pt{0.50f, 0.38f}, Pt{0.45f, 0.50f}, Pt{0.55f, 0.60f}, Pt{0.60f, 0.18f}, Pt{0.66f, 0.70f}, Pt{0.46f, 0.80f}, Pt{0.38f, 0.95f}});
    t.river = {{0.74f, 0.0f}, {0.72f, 0.16f}, {0.76f, 0.34f}, {0.80f, 0.52f}, {0.84f, 0.72f}, {0.90f, 1.0f}};
    streets(t, {"Shantipath", "Nyaya Marg", "Panchsheel Marg", "Janpath", "Barakhamba Road", "Kasturba Gandhi Marg",
                "Rajpath", "Rafi Marg", "Akbar Road", "Lodi Road", "Max Mueller Marg", "Jor Bagh Road",
                "Chandni Chowk", "Daryaganj", "Netaji Subhash Marg", "the Ring Road", "Lajpat Rai Marg", "Bhisham Pitamah Marg",
                "Aurobindo Marg", "Green Park Main Road", "the Outer Ring Road", "the Qutb road", "Chattarpur Road", "the Badarpur road"});
    fill(t.park, {"Nehru Park", "the Central Park", "the India Gate lawns", "the Lodi Gardens", "the Red Fort lawns", "the colony park", "the Deer Park", "the Qutb ruins"});
    fill(t.station, {"the Chanakyapuri bus stand", "the Regal cinema", "the Central Secretariat bus terminus", "the Lodi Colony railway halt",
                     "Old Delhi railway station", "the Defence Colony market", "the Green Park bus stand", "the Qutb Minar bus stand"});
    t.venue = {"the Coffee House", "Gaylord", "the United Coffee House", "Moti Mahal", "the Embassy restaurant", "the Volga", "Nirula's", "Wenger's tea-room", "Kwality", "the Standard"};
    t.meetT = {"%P, %S", "tea stall, %S", "club verandah, %S", "cinema foyer, %S", "hotel coffee shop, %S", "bench walk, %K"};
    t.dropT = {"loose stone, tomb wall in %K", "hollow in the old banyan, %K", "behind the cistern, lavatory at %T", "magnetic container under the culvert rail, %S",
               "drainpipe bracket, lane off %S", "under the marked milestone, %S"};
    t.signalT = {"chalk mark, letter-box, %S", "drawing-pin, bench in %K", "strip of tape, railing outside %T", "chalk mark, electricity pole, %S"};
    t.male = {"Rajesh", "Ashok", "Vijay", "Suresh", "Ramesh", "Anil", "Mohan", "Prakash", "Arun", "Krishna", "Harish", "Satish", "Dinesh", "Mahesh",
              "Naresh", "Vinod", "Pradeep", "Gopal", "Balraj", "Inder", "Kuldip", "Harbans", "Iqbal", "Salim", "Joseph", "James", "Robert",
              "William", "Richard", "Donald"};
    t.female = {"Sunita", "Asha", "Kamala", "Usha", "Meena", "Shanti", "Lakshmi", "Rekha", "Mary", "Helen"};
    t.last = {"Sharma", "Verma", "Gupta", "Mehta", "Kapoor", "Malhotra", "Khanna", "Chopra", "Singh", "Bhatia", "Nair", "Menon", "Iyer", "Rao",
              "Reddy", "Banerjee", "Mukherjee", "Chatterjee", "Sen", "Das", "Joshi", "Desai", "Patel", "Shah", "Agarwal", "Saxena", "Mathur",
              "Sinha", "Mishra", "Pandey", "Khan", "Ahmed", "Hussain", "D'Souza", "Fernandes", "Thompson", "Harris", "Walker", "Miller", "Baker"};
    t.jobs[POLITICAL] = {
        {"Private secretary to a minister", "the Government of India", 60, 88, 35, 20, false},
        {"Section officer", "the Ministry of External Affairs", 50, 78, 40, 15, false},
        {"Secretary to a member", "the Lok Sabha", 30, 55, 12, 55, false},
        {"Correspondent", "a Delhi English-language daily", 25, 50, 6, 90, false},
        {"Editor", "a weekly review", 20, 45, 5, 92, false},
        {"Official", "the Congress party headquarters", 35, 62, 12, 65, false},
        {"Second secretary", "the embassy of a Western state", 45, 72, 55, 20, false},
        {"Attaché", "the Chinese Embassy", 50, 80, 76, 5, true},
        {"First secretary", "the High Commission of Pakistan", 50, 78, 62, 10, true}};
    t.jobs[MILITARY] = {
        {"Staff officer", "Army Headquarters", 50, 78, 50, 5, true},
        {"Clerk", "the Ministry of Defence, South Block", 45, 75, 45, 5, false},
        {"Wing commander", "Air Headquarters", 50, 80, 55, 5, true},
        {"Sergeant", "the United States defence attaché's office", 55, 85, 72, 5, true},
        {"Engineer", "the liaison office of the state aircraft works", 40, 65, 35, 10, false},
        {"Defence correspondent", "a national daily", 25, 45, 8, 80, false}};
    t.jobs[SCITECH] = {
        {"Scientist", "the Department of Atomic Energy", 55, 88, 60, 20, false},
        {"Engineer", "the space research organisation", 45, 75, 45, 15, false},
        {"Official", "the Department of Electronics", 40, 65, 30, 10, false},
        {"Representative", "the Delhi office of a Western computer firm", 40, 70, 30, 10, false},
        {"Chemist", "a pharmaceuticals laboratory", 25, 50, 10, 5, false},
        {"Librarian", "the United States Information Service", 25, 50, 35, 15, false}};
    t.jobs[SECURITY] = {
        {"Deputy central intelligence officer", "the Intelligence Bureau", 55, 82, 62, 5, true},
        {"Clerk", "the Foreigners' Regional Registration Office", 35, 60, 25, 5, false},
        {"Communications clerk", "the United States Embassy", 62, 92, 80, 5, false},
        {"Local employee", "the British High Commission", 35, 62, 45, 5, false},
        {"Inspector", "Delhi Police Special Branch", 40, 65, 35, 5, true},
        {"Private enquiry agent", "self-employed (ex-police)", 25, 50, 5, 15, true}};
    t.spies = {
        {"Officer, counter-intelligence", "the Intelligence Bureau", 62, 88, SVC_SECURITY},
        {"Watcher, surveillance unit", "the Intelligence Bureau", 42, 65, SVC_SECURITY},
        {"Second secretary (a station officer)", "the United States Embassy", 70, 93, SVC_ADVERSARY},
        {"First secretary (a station officer)", "the British High Commission", 65, 88, SVC_ADVERSARY},
        {"Officer", "the Research and Analysis Wing", 55, 82, SVC_THIRD},
        {"Attaché (an officer of Pakistani intelligence)", "the High Commission of Pakistan", 50, 78, SVC_THIRD}};
    t.reqs = {
        {SCITECH, "The Indian nuclear programme", "Intentions, timetable and technical state of work on a nuclear explosive device; plutonium stocks; the attitude of the Prime Minister's office.", 0, 220, 0, RK_COLLECT},
        {SECURITY, "The American Embassy", "Communications, personnel and methods of the main adversary's station in Delhi; its sources in the Indian government and press.", 0, 240, 0, RK_COLLECT},
        {POLITICAL, "Service A: the main adversary in India", "ACTIVE MEASURE. Place material in the Indian press on American interference in Indian politics and American support for the opposition. Three placements required.", 1, 3, 240, RK_ACTIVE},
        {POLITICAL, "The Chinese Embassy", "Activities and reporting of the Chinese mission; Peking's approaches to Delhi and to the Indian left.", 0, 180, 0, RK_COLLECT},
        {MILITARY, "Western arms offers", "Western proposals to supply strike aircraft and tanks; the views of Air and Army Headquarters; who is paid by whom.", 3, 180, 0, RK_COLLECT},
        {POLITICAL, "Indo-Pakistani relations", "Indian intentions in the talks with Pakistan; the Pakistani mission's assessments; Kashmir.", 5, 150, 200, RK_COLLECT},
        {MILITARY, "The American base in the Indian Ocean", "Indian knowledge of, and reaction to, the expansion of the base on Diego Garcia; American naval movements.", 8, 170, 0, RK_COLLECT},
        {POLITICAL, "Service A: the Indian Ocean", "ACTIVE MEASURE. Place material in the Indian press and Parliament against the American base on Diego Garcia. Three placements required.", 14, 3, 240, RK_ACTIVE},
        {POLITICAL, "The internal situation", "The Prime Minister's intentions toward the opposition movement; the cohesion of the Congress; the role of her son.", 18, 160, 150, RK_COLLECT},
        {SECURITY, "The Intelligence Bureau", "Organisation and methods of Indian counter-intelligence against the Soviet colony; liaison with Western services.", 10, 150, 0, RK_COLLECT},
        {SCITECH, "Western computers and electronics", "Western equipment supplied to Indian establishments; documentation and samples obtainable locally.", 12, 180, 0, RK_COLLECT},
        {POLITICAL, "The emergency", "How long the emergency is to last; intended constitutional changes; Western embassies' reporting.", 23, 170, 200, RK_COLLECT},
        {POLITICAL, "The general election", "A forecast of the result of the general election, with the evidence for it. Centre will act on your assessment.", 41, 140, 60, RK_COLLECT},
        {POLITICAL, "Service A: forged cables", "ACTIVE MEASURE. Place Service A's documents on American plans against the Prime Minister with editors who will print them. Three placements required.", 26, 3, 240, RK_ACTIVE},
        {MILITARY, "The Indian Navy", "The Navy's foreign purchases and its exercises with Western fleets.", 28, 160, 0, RK_COLLECT},
        {SECURITY, "The Research and Analysis Wing", "Personnel, liaison and operations of the external service.", 30, 170, 0, RK_COLLECT}};
    t.hist = {
        {1973, 10, 6, "Embassy bulletin", "War in the Middle East", "Egypt and Syria have attacked Israel. The Americans in Delhi will be reporting the Indian reaction and the oil position; so should we.", 0, 0, 0, H_NONE},
        {1973, 11, 26, "Rezident", "The General Secretary in Delhi", "L. I. Brezhnev arrives in Delhi on a state visit. The Intelligence Bureau and the police are wholly taken up with his protection. For a week nobody in this city has time to follow a Second Secretary.", 0, -0.35f, -2, H_NONE},
        {1974, 5, 18, "Centre", "Indian nuclear explosion", "India has exploded a nuclear device underground in Rajasthan. Centre learned of it from the wire services. An explanation is required of why the rezidentura gave no warning, and every source with access to the atomic establishment is to be put to work.", 0, 0, 0, H_NONE},
        {1975, 6, 26, "Rezident", "State of emergency", "The President has proclaimed a state of emergency. The opposition leaders were arrested in the night; the newspapers are under censorship. The police have other work than us for the present - but editors will print nothing that has not been passed, and that includes what we give them.", -0.05f, -0.3f, 0, H_NONE},
        {1975, 8, 15, "Embassy bulletin", "Coup in Dacca", "The President of Bangladesh has been killed with his family in a military coup. Delhi sees the Americans behind it. Centre wants the evidence, if there is any.", 0, 0, 0, H_NONE},
        {1976, 9, 9, "Embassy bulletin", "Death of Mao", "Mao Tse-tung is dead. The Chinese mission is the best window on the succession that this rezidentura has.", 0, 0, 0, H_NONE},
        {1977, 1, 18, "Embassy bulletin", "Elections called", "The Prime Minister has announced a general election for March and released the opposition leaders. Centre requires a forecast.", 0, 0, 0, H_NONE},
        {1977, 3, 22, "Centre", "The election result", "", 0.1f, 0, 0, H_JANATA}};
    t.local = "India is the friendliest ground you will ever work on. The Intelligence Bureau watches us because it must, and not very hard. Half the people you meet will offer to sell you something before the second whisky. That is the difficulty: Centre is tired of paying for what it could read in the Hindustan Times. The Americans, the Chinese and the atomic establishment are what will make your name here.";
    t.tour = "CENTRE, 1972-1973\n\nYou spent twenty months at Yasenevo, at a desk in the Seventeenth Department - South Asia - which was not the desk you had asked for. The personnel directorate had read the file on your London tour and drawn its conclusions: a quiet station, a friendly country, and time to show what you can do.\n\nIn the summer of 1973 you were posted to New Delhi.";
    return t;
}

// ============================================================================ BERLIN-KARLSHORST (GDR), 1979-84 and 1986-91
// The alternative to Washington and to Bonn: the KGB's largest station abroad, on friendly soil, working
// against the Western sectors across the Wall. In the East nobody follows you; your agents have to cross.
static Theatre berlinBase() {
    Theatre t{};
    Theatre de = bonn();
    t.male = de.male; t.female = de.female; t.last = de.last;
    t.cityRu = "БЕРЛИН"; t.friends = true;
    t.city = "Berlin"; t.country = "the Western sectors of Berlin";
    t.embassy = "KGB Representation, Zwieseler Strasse, Berlin-Karlshorst";
    t.adversary = "the Allied security services and the West Berlin political police"; t.watchers = "West Berlin police and Allied"; t.currency = "marks";
    t.chief = "Head of the Representation";
    t.lang = 1;
    fill(t.district, {"Karlshorst", "Mitte", "Pankow", "Köpenick", "Kreuzberg", "Charlottenburg", "Wedding", "Zehlendorf"});
    fill(t.east, {true, true, true, true, false, false, false, false});
    fill(t.dist, {0, 1, 1, 1, 2, 3, 2, 3});
    fill(t.density, {0.0f, 0.0f, 0.0f, 0.0f, 0.24f, 0.30f, 0.12f, 0.14f});
    fill(t.at, {Pt{0.84f, 0.62f}, Pt{0.60f, 0.42f}, Pt{0.62f, 0.14f}, Pt{0.92f, 0.82f}, Pt{0.50f, 0.60f}, Pt{0.26f, 0.44f}, Pt{0.42f, 0.20f}, Pt{0.16f, 0.82f}});
    t.river = {{0.0f, 0.36f}, {0.16f, 0.38f}, {0.34f, 0.36f}, {0.50f, 0.40f}, {0.62f, 0.48f}, {0.74f, 0.58f}, {0.86f, 0.72f}, {1.0f, 0.78f}};
    t.wall = {{0.50f, 0.0f}, {0.53f, 0.14f}, {0.50f, 0.28f}, {0.54f, 0.38f}, {0.52f, 0.48f}, {0.58f, 0.56f}, {0.64f, 0.66f}, {0.62f, 0.80f}, {0.68f, 1.0f}};
    streets(t, {"Treskowallee", "Ehrenfelsstraße", "Rheinsteinstraße", "Friedrichstraße", "Unter den Linden", "Karl-Liebknecht-Straße",
                "Breite Straße", "Schönholzer Straße", "Wollankstraße", "Bahnhofstraße", "Müggelheimer Straße", "Grünstraße",
                "Oranienstraße", "Mehringdamm", "Bergmannstraße", "Kantstraße", "the Kurfürstendamm", "Bismarckstraße",
                "Müllerstraße", "Seestraße", "Badstraße", "Clayallee", "Teltower Damm", "Argentinische Allee"});
    fill(t.park, {"the racecourse grounds", "the Monbijou park", "the Schlosspark", "the Müggelsee shore", "the Viktoriapark", "the Lietzensee park", "the Humboldthain", "the Grunewald"});
    fill(t.station, {"Karlshorst S-Bahn station", "Friedrichstraße station", "Pankow S-Bahn station", "Köpenick S-Bahn station",
                     "Kottbusser Tor U-Bahn station", "Zoo station", "Gesundbrunnen station", "Krumme Lanke U-Bahn station"});
    t.venue = {"Gaststätte zur Linde", "Café Moskau", "Zum Nußbaum", "Café Kranzler", "Bierstube Alt-Berlin", "Weinstube Habel", "Café Möhring",
               "Zur Letzten Instanz", "Gasthaus Krone", "Eckkneipe Zum Anker"};
    t.meetT = {"%P, %S", "Konditorei, %S", "corner pub, %S", "cinema foyer, %S", "hotel bar, %S", "bench walk, %K"};
    t.dropT = {"loose brick, cemetery wall off %S", "hollow behind the bench, %K", "behind the timetable case, %T", "magnetic container under the bridge rail, %S",
               "coal-cellar grating, courtyard off %S", "under the marked stone, path in %K"};
    t.signalT = {"chalk mark, advertising column, %S", "drawing-pin, bench in %K", "strip of tape, railing outside %T", "chalk mark, hydrant, %S"};
    t.jobs[POLITICAL] = {
        {"Official", "the Senate Chancellery of West Berlin", 55, 82, 60, 15, false},
        {"Political adviser", "the United States Mission, Berlin", 62, 90, 78, 10, false},
        {"Clerk", "the British Military Government", 55, 82, 70, 5, false},
        {"Editor", "the American radio station in the Western sectors", 35, 60, 40, 85, false},
        {"Reporter", "a West Berlin daily", 25, 50, 6, 85, false},
        {"Aide", "a party group in the House of Representatives", 30, 58, 12, 55, false},
        {"Official", "the Federal Plenipotentiary's office", 50, 78, 58, 15, false},
        {"Organiser", "the Alternative List", 20, 45, 5, 65, false}};
    t.jobs[MILITARY] = {
        {"Linguist", "the Allied listening station on the Teufelsberg", 70, 95, 84, 5, false},
        {"Sergeant, operations section", "the United States Berlin Brigade", 55, 82, 70, 5, true},
        {"Clerk", "the Allied Staff, Berlin", 60, 88, 74, 5, false},
        {"Technician", "the signals unit at Tempelhof", 60, 88, 76, 5, true},
        {"Driver-interpreter", "the French military government", 40, 65, 50, 5, true},
        {"Liaison NCO", "the British military mission", 50, 78, 66, 5, true},
        {"Civilian employee", "a United States Army supply depot", 35, 60, 40, 5, false}};
    t.jobs[SCITECH] = {
        {"Engineer", "a West Berlin electrical concern", 50, 82, 45, 10, false},
        {"Chemist", "a West Berlin pharmaceuticals firm", 40, 68, 40, 10, false},
        {"Researcher", "the Technical University", 35, 62, 10, 20, false},
        {"Physicist", "the nuclear research institute at Wannsee", 50, 80, 50, 20, false},
        {"Export clerk", "a forwarding agent at the Tegel freight terminal", 45, 75, 15, 5, false},
        {"Programmer", "the West Berlin office of an American computer firm", 45, 78, 40, 10, false},
        {"Technician", "a telecommunications works", 40, 68, 35, 5, true}};
    t.jobs[SECURITY] = {
        {"Clerk", "the State Office for the Protection of the Constitution", 60, 88, 82, 5, false},
        {"Detective", "the West Berlin political police", 50, 78, 62, 5, true},
        {"Interviewer", "the refugee reception centre at Marienfelde", 50, 80, 60, 5, false},
        {"Customs officer", "the transit checkpoint at Dreilinden", 35, 60, 40, 5, true},
        {"Local guard", "the United States Mission", 40, 68, 55, 5, true},
        {"Private detective", "self-employed (ex-police)", 25, 50, 5, 15, true}};
    t.spies = {
        {"Case officer", "the American intelligence base in Berlin", 75, 96, SVC_ADVERSARY},
        {"Agent, counter-intelligence", "United States Army intelligence, Berlin", 62, 88, SVC_SECURITY},
        {"Officer", "the State Office for the Protection of the Constitution", 65, 90, SVC_SECURITY},
        {"Officer", "the British security organisation in Berlin", 62, 88, SVC_SECURITY},
        {"Officer", "the Berlin outstation of the Federal Intelligence Service", 65, 90, SVC_ADVERSARY},
        {"Officer", "French military security", 45, 72, SVC_THIRD}};
    t.local = "You live and work in the East, among friends. Nobody follows you in Karlshorst, Mitte, Pankow or Köpenick, and a meeting there cannot be watched by the adversary. But the people worth meeting live in the Western sectors. Either you cross to them - under the eyes of the Allied services and the West Berlin police, without a diplomat's protection - or they cross to you, and every crossing is written down by somebody. The friends in the Normannenstrasse answer a trace in days. Remember that whatever they are asked, they file.";
    return t;
}

static Theatre berlin1() {
    Theatre t = berlinBase();
    t.slot = 2;
    t.cover = "Second Secretary, Soviet Embassy to the GDR";
    t.sy = 1979; t.sm = 9; t.sd = 3; t.ey = 1984; t.em = 6; t.ed = 29;
    t.role = 2; t.subs = 3; t.colleagues = 6;
    t.price = 3; t.a4Base = 1.0f; t.heat0 = 16; t.blownHeat = 4; t.dangle = 0.13f; t.walkIn = 0.45f; t.valuePar = 2000;
    t.reqs = {
        {MILITARY, "The Teufelsberg station", "Tasking, equipment and product of the American and British listening station on the Teufelsberg: what it hears of the Group of Soviet Forces and of the GDR.", 0, 300, 0, RK_COLLECT},
        {POLITICAL, "Allied contingency planning for Berlin", "Tripartite plans for a crisis over the access routes; the papers of the 'Live Oak' staff.", 0, 240, 0, RK_COLLECT},
        {SECURITY, "Western services in Berlin", "Bases, safe houses, personnel and agent-running methods of the American, British and West German services working against the GDR from the Western sectors.", 0, 220, 0, RK_COLLECT},
        {SCITECH, "Microelectronics", "Process documentation and embargoed production equipment for integrated circuits, obtainable through West Berlin firms and forwarding agents.", 0, 260, 0, RK_COLLECT},
        {MILITARY, "The Allied garrisons", "Order of battle, alert procedures and reinforcement plans of the three Western garrisons.", 6, 200, 0, RK_COLLECT},
        {SECURITY, "Escape-helper organisations", "Identity, methods and funding of the groups smuggling GDR citizens to the West.", 8, 150, 0, RK_COLLECT},
        {POLITICAL, "The Senate and the house occupations", "The governing crisis in the West Berlin Senate; the occupation movement, its leaders and its money.", 14, 130, 180, RK_COLLECT},
        {SCITECH, "Embargoed computers", "Channels for the purchase of American minicomputers through West Berlin intermediaries.", 16, 220, 0, RK_COLLECT},
        {POLITICAL, "Service A: the missiles and the city", "ACTIVE MEASURE. Place material with West Berlin journalists and peace groups on what the new American missiles mean for the city. Three placements required.", 20, 3, 300, RK_ACTIVE},
        {MILITARY, "RYAN: Berlin indicators", "Indicators of preparation for a surprise attack observable in the Western garrisons: alert states, plans to evacuate dependants, fuel and blood stocks.", 22, 180, 0, RK_COLLECT},
        {SECURITY, "Interrogation of refugees", "What the Western services learn at the Marienfelde reception centre, and how they use it.", 26, 160, 0, RK_COLLECT},
        {POLITICAL, "The American President's visit", "Programme, security arrangements and political purpose of the President's visit to the Western sectors.", 31, 110, 70, RK_COLLECT},
        {POLITICAL, "Inner-German contacts", "The intentions of Bonn and of the Senate in their dealings with the GDR; the billion-mark credit.", 40, 150, 180, RK_COLLECT},
        {POLITICAL, "Service A: the American radio station", "ACTIVE MEASURE. Place material in the West Berlin press on the staffing and financing of the American radio station. Three placements required.", 44, 3, 300, RK_ACTIVE}};
    t.hist = {
        {1979, 12, 12, "Embassy bulletin", "The NATO decision", "NATO has decided to station new American missiles in Europe unless negotiations succeed. West Berlin will be a centre of the movement against them; Service A will have work for us.", 0, 0, 0, H_NONE},
        {1979, 12, 27, "Head of the Representation", "Afghanistan", "A limited contingent of Soviet forces has entered Afghanistan at the request of its government. Expect harder treatment at the crossing points and a hostile press in the Western sectors. Centre wants Allied intentions at once.", 0.05f, 0, 2, H_NONE},
        {1980, 12, 12, "Embassy bulletin", "Riots in Kreuzberg", "Street fighting in Kreuzberg after the police cleared occupied houses. Every policeman in the Western sectors is on the street; nobody is watching the U-Bahn exits.", 0, -0.3f, 0, H_NONE},
        {1981, 5, 25, "Centre", "Operation RYAN", "", 0, 0, 0, H_RYAN},
        {1981, 9, 22, "Embassy bulletin", "Death of a demonstrator", "A young man was killed under a bus when the police cleared eight occupied houses. Barricades in Schöneberg. The political police have no men to spare for some days.", 0, -0.35f, 0, H_NONE},
        {1982, 6, 11, "Embassy bulletin", "The American President in Berlin", "The American President is in the Western sectors today. Ten thousand police, Allied military police on every corner, and helicopters. No operational acts in the West this week.", 0, 0.7f, 0, H_NONE},
        {1982, 11, 10, "Centre", "Death of L. I. Brezhnev", "The General Secretary is dead. Comrade Yu. V. Andropov, for fifteen years Chairman of this Committee, succeeds him. Reporting will now be read at the very top, and read by a professional.", 0, 0, 0, H_NONE},
        {1983, 9, 1, "Head of the Representation", "The Korean airliner", "A South Korean airliner has been destroyed in Soviet airspace off Sakhalin. Demonstrations at the crossing points. Reduce acts in the West to those that cannot wait.", 0.15f, 0.3f, 3, H_NONE},
        {1983, 11, 7, "Centre", "FLASH - RYAN", "", 0, 0, 0, H_ABLE},
        {1984, 2, 9, "Centre", "Death of Yu. V. Andropov", "The General Secretary is dead. Work continues under the existing instructions.", 0, 0, 0, H_NONE}};
    t.tour = "CENTRE, 1977-1979\n\nTwo years at Yasenevo, and not in the department you wanted. The Americans do not give visas to officers whose photographs are in their files, and the First Department had other candidates. You worked in the Fourth - the two Germanies - reading what Karlshorst sent in and learning how much of it came from the friends.\n\nIn 1979 you were posted to the Representation in Berlin, as head of a line working the Western sectors.";
    return t;
}

static Theatre berlin2() {
    Theatre t = berlinBase();
    t.slot = 3;
    t.cover = "First Secretary, Soviet Embassy to the GDR";
    t.sy = 1986; t.sm = 9; t.sd = 1; t.ey = 1991; t.em = 6; t.ed = 28;
    t.role = 2; t.subs = 4; t.colleagues = 5;
    t.price = 4; t.a4Base = 0.95f; t.heat0 = 18; t.blownHeat = 4; t.dangle = 0.12f; t.walkIn = 0.45f; t.valuePar = 3200;
    t.reqs = {
        {MILITARY, "The Teufelsberg station", "Tasking, equipment and product of the Allied listening station on the Teufelsberg; what it has learned of the new command systems of the Group of Soviet Forces.", 0, 300, 0, RK_COLLECT},
        {SECURITY, "Western services in Berlin", "Bases, personnel and agent-running methods of the American, British and West German services working from the Western sectors.", 0, 240, 0, RK_COLLECT},
        {SCITECH, "Semiconductor production equipment", "Lithography, clean-room and test equipment of the newest generation; documentation, and channels through West Berlin.", 0, 280, 0, RK_COLLECT},
        {POLITICAL, "Allied views of the Soviet reforms", "How the three Western missions and the Senate judge perestroika and what they report home.", 0, 170, 0, RK_COLLECT},
        {POLITICAL, "Service A: the anniversary year", "ACTIVE MEASURE. Place material in the West Berlin press on the cost and the politics of the Senate's 750th-anniversary celebrations. Three placements required.", 2, 3, 300, RK_ACTIVE},
        {MILITARY, "The Allied garrisons", "Order of battle and planning of the Western garrisons under reduced tension.", 4, 200, 0, RK_COLLECT},
        {SCITECH, "Research networks", "Access to computers of Western research establishments through the open data networks.", 5, 240, 0, RK_COLLECT},
        {POLITICAL, "The American President's speech", "Advance text and purpose of the American President's speech at the Brandenburg Gate.", 8, 110, 50, RK_COLLECT},
        {POLITICAL, "Inner-German relations", "Bonn's and the Senate's intentions towards the GDR leadership; credits; the visit of the General Secretary of the SED to Bonn.", 10, 160, 200, RK_COLLECT},
        {SECURITY, "Emigration and the Western services", "Use made by the Western services of GDR citizens leaving through Hungary and the embassies.", 34, 170, 150, RK_COLLECT},
        {SECURITY, "The opened frontier", "How the Western services are exploiting the opened frontier: recruitment of GDR officials, purchase of documents, approaches to our own people.", 39, 220, 0, RK_COLLECT},
        {POLITICAL, "The German question: Allied positions", "The real positions of the three Western powers on German unity, alliance membership and the future of their garrisons.", 40, 260, 240, RK_COLLECT},
        {SECURITY, "The files of the friends", "What the Western services have obtained of the records of the Ministry for State Security, and whom they name.", 42, 220, 0, RK_COLLECT},
        {MILITARY, "Withdrawal of the Western Group of Forces", "Western intentions and intelligence collection against the withdrawal of Soviet forces from Germany.", 46, 220, 0, RK_COLLECT}};
    t.hist = {
        {1986, 10, 12, "Embassy bulletin", "Reykjavik", "The meeting between the General Secretary and the American President in Iceland ended without agreement, but everything is now on the table. Allied reactions are required.", 0, 0, 0, H_NONE},
        {1987, 6, 12, "Embassy bulletin", "The American President at the Brandenburg Gate", "The American President spoke today on the Western side of the Brandenburg Gate and called on the General Secretary to tear down the Wall. Twenty-five thousand police. No operational acts in the West this week.", 0, 0.7f, 0, H_NONE},
        {1987, 9, 7, "Embassy bulletin", "The General Secretary of the SED in Bonn", "The East German leader begins an official visit to the Federal Republic. He believes it is the summit of his life's work. The friends are in excellent spirits.", 0, -0.1f, 0, H_NONE},
        {1989, 9, 11, "Head of the Representation", "Hungary opens its frontier", "Hungary has opened its frontier to Austria for citizens of the GDR. They are leaving by the thousand. The friends ask us daily what Moscow intends. We have no instructions to give them.", 0, 0, 0, H_NONE},
        {1989, 10, 9, "Embassy bulletin", "Leipzig", "Seventy thousand people marched through Leipzig last night. The security forces had orders and did not carry them out. Our forces stayed in barracks.", 0, 0, 0, H_NONE},
        {1989, 10, 18, "Embassy bulletin", "Change of leadership", "The General Secretary of the SED has been relieved of his offices 'at his own request'. Nobody in the Normannenstrasse can tell us who is now in charge of what.", 0, 0, 0, H_NONE},
        {1989, 11, 9, "Embassy bulletin", "The frontier is open", "", -0.1f, -0.3f, 0, H_WALL},
        {1990, 1, 15, "Line KR", "The Normannenstrasse", "", 0.1f, 0, 0, H_ORPHANS},
        {1990, 7, 1, "Embassy bulletin", "Monetary union", "The West German mark is from today the currency of the GDR. The state that is our host is ceasing to exist by the week; its officers are looking for new employers, and finding them.", 0, 0, 0, H_NONE},
        {1990, 10, 3, "Head of the Representation", "Unification", "", 0.25f, 0, 6, H_UNIFY}};
    t.tour = "CENTRE, 1984-1986\n\nTwo years in Directorate K, external counter-intelligence, and they were the years to be there. In 1985 one agent after another inside the Committee was uncovered and brought home; nobody at your level was told how. You read the damage assessments. Every one of them was about who had known what.\n\nNo Western government would now accept you as a diplomat. In 1986 you went back to Karlshorst, to head the department that works the Western sectors.";
    return t;
}

// The order here must match the TH_ numbers in content.h.
const std::vector<Theatre> THEATRES = {moscow(), london(), vienna(), delhi(), washington(), berlin1(), bonn(), berlin2()};
