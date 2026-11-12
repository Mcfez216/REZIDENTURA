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
    t.tour = "CENTRE, 1972-1973\n\nYou spent twenty months at Yasenevo: a desk in the Third Department, reading other officers' cables from Britain and Scandinavia, and a course at the Red Banner Institute. You learned how a cable from the field looks from the receiving end - how little of it is read, and by whom.\n\nIn the summer of 1973 the personnel directorate offered you Vienna.";
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
    t.tour = "CENTRE, 1984-1986\n\nYou served two years in Directorate K, external counter-intelligence, and they were the years to be there. In 1985 one agent after another inside the Committee and the military neighbours was uncovered and brought home; nobody at your level was told how. In the same summer the acting Rezident in London was recalled, questioned, and then vanished from under surveillance in Moscow - to reappear in Britain. You read the damage assessments. Every one of them was about who had known what.\n\nIn 1986 you were appointed Rezident in Bonn.";
    return t;
}

const std::vector<Theatre> THEATRES = {london(), vienna(), washington(), bonn()};
