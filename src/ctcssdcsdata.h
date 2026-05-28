#pragma once
#include <QMap>
#include <QString>
#include <QVector>

struct CtcssDcsEntry {
    int code;
    QString label;
};

inline QVector<CtcssDcsEntry> buildCtcssDcsList() {
    QVector<CtcssDcsEntry> list;
    list.append({0,   "None / All"});
    list.append({64,  "CTCSS 67.0 Hz"});
    list.append({65,  "CTCSS 69.3 Hz"});
    list.append({66,  "CTCSS 71.9 Hz"});
    list.append({67,  "CTCSS 74.4 Hz"});
    list.append({68,  "CTCSS 77.0 Hz"});
    list.append({69,  "CTCSS 79.7 Hz"});
    list.append({70,  "CTCSS 82.5 Hz"});
    list.append({71,  "CTCSS 85.4 Hz"});
    list.append({72,  "CTCSS 88.5 Hz"});
    list.append({73,  "CTCSS 91.5 Hz"});
    list.append({74,  "CTCSS 94.8 Hz"});
    list.append({75,  "CTCSS 97.4 Hz"});
    list.append({76,  "CTCSS 100.0 Hz"});
    list.append({77,  "CTCSS 103.5 Hz"});
    list.append({78,  "CTCSS 107.2 Hz"});
    list.append({79,  "CTCSS 110.9 Hz"});
    list.append({80,  "CTCSS 114.8 Hz"});
    list.append({81,  "CTCSS 118.8 Hz"});
    list.append({82,  "CTCSS 123.0 Hz"});
    list.append({83,  "CTCSS 127.3 Hz"});
    list.append({84,  "CTCSS 131.8 Hz"});
    list.append({85,  "CTCSS 136.5 Hz"});
    list.append({86,  "CTCSS 141.3 Hz"});
    list.append({87,  "CTCSS 146.2 Hz"});
    list.append({88,  "CTCSS 151.4 Hz"});
    list.append({89,  "CTCSS 156.7 Hz"});
    list.append({90,  "CTCSS 159.8 Hz"});
    list.append({91,  "CTCSS 162.2 Hz"});
    list.append({92,  "CTCSS 165.5 Hz"});
    list.append({93,  "CTCSS 167.9 Hz"});
    list.append({94,  "CTCSS 171.3 Hz"});
    list.append({95,  "CTCSS 173.8 Hz"});
    list.append({96,  "CTCSS 177.3 Hz"});
    list.append({97,  "CTCSS 179.9 Hz"});
    list.append({98,  "CTCSS 183.5 Hz"});
    list.append({99,  "CTCSS 186.2 Hz"});
    list.append({100, "CTCSS 189.9 Hz"});
    list.append({101, "CTCSS 192.8 Hz"});
    list.append({102, "CTCSS 196.6 Hz"});
    list.append({103, "CTCSS 199.5 Hz"});
    list.append({104, "CTCSS 203.5 Hz"});
    list.append({105, "CTCSS 206.5 Hz"});
    list.append({106, "CTCSS 210.7 Hz"});
    list.append({107, "CTCSS 218.1 Hz"});
    list.append({108, "CTCSS 225.7 Hz"});
    list.append({109, "CTCSS 229.1 Hz"});
    list.append({110, "CTCSS 233.6 Hz"});
    list.append({111, "CTCSS 241.8 Hz"});
    list.append({112, "CTCSS 250.3 Hz"});
    list.append({113, "CTCSS 254.1 Hz"});
    list.append({127, "Search"});
    list.append({128, "DCS 023"});
    list.append({129, "DCS 025"});
    list.append({130, "DCS 026"});
    list.append({131, "DCS 031"});
    list.append({132, "DCS 032"});
    list.append({133, "DCS 036"});
    list.append({134, "DCS 043"});
    list.append({135, "DCS 047"});
    list.append({136, "DCS 051"});
    list.append({137, "DCS 053"});
    list.append({138, "DCS 054"});
    list.append({139, "DCS 065"});
    list.append({140, "DCS 071"});
    list.append({141, "DCS 072"});
    list.append({142, "DCS 073"});
    list.append({143, "DCS 074"});
    list.append({144, "DCS 114"});
    list.append({145, "DCS 115"});
    list.append({146, "DCS 116"});
    list.append({147, "DCS 122"});
    list.append({148, "DCS 125"});
    list.append({149, "DCS 131"});
    list.append({150, "DCS 132"});
    list.append({151, "DCS 134"});
    list.append({152, "DCS 143"});
    list.append({153, "DCS 145"});
    list.append({154, "DCS 152"});
    list.append({155, "DCS 155"});
    list.append({156, "DCS 156"});
    list.append({157, "DCS 162"});
    list.append({158, "DCS 165"});
    list.append({159, "DCS 172"});
    list.append({160, "DCS 174"});
    list.append({161, "DCS 205"});
    list.append({162, "DCS 212"});
    list.append({163, "DCS 223"});
    list.append({164, "DCS 225"});
    list.append({165, "DCS 226"});
    list.append({166, "DCS 243"});
    list.append({167, "DCS 244"});
    list.append({168, "DCS 245"});
    list.append({169, "DCS 246"});
    list.append({170, "DCS 251"});
    list.append({171, "DCS 252"});
    list.append({172, "DCS 255"});
    list.append({173, "DCS 261"});
    list.append({174, "DCS 263"});
    list.append({175, "DCS 265"});
    list.append({176, "DCS 266"});
    list.append({177, "DCS 271"});
    list.append({178, "DCS 274"});
    list.append({179, "DCS 306"});
    list.append({180, "DCS 311"});
    list.append({181, "DCS 315"});
    list.append({182, "DCS 325"});
    list.append({183, "DCS 331"});
    list.append({184, "DCS 332"});
    list.append({185, "DCS 343"});
    list.append({186, "DCS 346"});
    list.append({187, "DCS 351"});
    list.append({188, "DCS 356"});
    list.append({189, "DCS 364"});
    list.append({190, "DCS 365"});
    list.append({191, "DCS 371"});
    list.append({192, "DCS 411"});
    list.append({193, "DCS 412"});
    list.append({194, "DCS 413"});
    list.append({195, "DCS 423"});
    list.append({196, "DCS 431"});
    list.append({197, "DCS 432"});
    list.append({198, "DCS 445"});
    list.append({199, "DCS 446"});
    list.append({200, "DCS 452"});
    list.append({201, "DCS 454"});
    list.append({202, "DCS 455"});
    list.append({203, "DCS 462"});
    list.append({204, "DCS 464"});
    list.append({205, "DCS 465"});
    list.append({206, "DCS 466"});
    list.append({207, "DCS 503"});
    list.append({208, "DCS 506"});
    list.append({209, "DCS 516"});
    list.append({210, "DCS 523"});
    list.append({211, "DCS 526"});
    list.append({212, "DCS 532"});
    list.append({213, "DCS 546"});
    list.append({214, "DCS 565"});
    list.append({215, "DCS 606"});
    list.append({216, "DCS 612"});
    list.append({217, "DCS 624"});
    list.append({218, "DCS 627"});
    list.append({219, "DCS 631"});
    list.append({220, "DCS 632"});
    list.append({221, "DCS 654"});
    list.append({222, "DCS 662"});
    list.append({223, "DCS 664"});
    list.append({224, "DCS 703"});
    list.append({225, "DCS 712"});
    list.append({226, "DCS 723"});
    list.append({227, "DCS 731"});
    list.append({228, "DCS 732"});
    list.append({229, "DCS 734"});
    list.append({230, "DCS 743"});
    list.append({231, "DCS 754"});
    list.append({240, "No Tone"});
    return list;
}

inline const QVector<CtcssDcsEntry>& ctcssDcsList() {
    static QVector<CtcssDcsEntry> list = buildCtcssDcsList();
    return list;
}

inline QString ctcssDcsLabel(int code) {
    for (const auto& e : ctcssDcsList()) {
        if (e.code == code) return e.label;
    }
    return QString("Code %1").arg(code);
}

// Channel index (1-500) to bank (1-10) and channel-within-bank (1-50)
inline int channelIndex(int bank, int ch) { return (bank - 1) * 50 + ch; }
inline int bankFromIndex(int idx)         { return (idx - 1) / 50 + 1; }
inline int chInBankFromIndex(int idx)     { return (idx - 1) % 50 + 1; }

// Delay time values
inline QVector<int> delayValues() { return {-10, -5, 0, 1, 2, 3, 4, 5}; }
inline QString delayLabel(int v) {
    if (v == 0) return "0s (off)";
    if (v < 0)  return QString("%1s (resume)").arg(v);
    return QString("+%1s").arg(v);
}
