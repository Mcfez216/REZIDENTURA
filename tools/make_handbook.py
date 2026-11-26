#!/usr/bin/env python3
"""make_handbook.py - sets manual.txt as "Officer's Handbook.pdf", made up as a file from the registry.

    python3 tools/make_handbook.py [manual.txt] [out.pdf]

Needs reportlab and Pillow (pip install reportlab pillow). The typefaces are the game's own, from fonts/;
the emblem is read out of src/emblem.h. Everything in the text comes from manual.txt: change the manual,
run this again.
"""
import io
import os
import re
import sys
from collections import Counter
from xml.sax.saxutils import escape

from PIL import Image
from reportlab.lib.colors import Color, HexColor
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, Frame, HRFlowable, NextPageTemplate, PageBreak, PageTemplate,
                                Paragraph, Spacer, Table, TableStyle)
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "manual.txt")
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "Officer's Handbook.pdf")
VERSION = "1.6"

PAPER, INK, FADED, RED = HexColor("#F6F0DF"), HexColor("#26221E"), HexColor("#6F665A"), HexColor("#A9231C")
RULE, MANILA, MANILA_D, PENCIL, VIOLET = HexColor("#B9AD93"), HexColor("#D6BC86"), HexColor("#B99A5E"), HexColor("#3B4A8A"), HexColor("#5B3F8C")
W, H = A4
LM, RM, TM, BM = 30 * mm, 19 * mm, 27 * mm, 23 * mm

for name, file in [("Mono", "IBMPlexMono-Regular.ttf"), ("MonoM", "IBMPlexMono-Medium.ttf"), ("MonoB", "IBMPlexMono-SemiBold.ttf"),
                   ("Narrow", "PT_Sans-Narrow-Web-Bold.ttf"), ("Serif", "PT_Serif-Web-Bold.ttf"), ("Hand", "BadScript-Regular.ttf"),
                   ("Display", "StalinistOne-Regular.ttf"), ("Russo", "RussoOne-Regular.ttf")]:
    pdfmetrics.registerFont(TTFont(name, os.path.join(ROOT, "fonts", file)))


def emblem(colour, size=420):
    """The hammer and sickle from src/emblem.h (a run-length coded mask), as a picture in one colour."""
    text = open(os.path.join(ROOT, "src", "emblem.h")).read()
    n = int(re.search(r"EMBLEM_SIZE\s*=\s*(\d+)", text).group(1))
    nums = [int(x) for x in re.findall(r"\d+", text[text.index("EMBLEM_RLE"):])]
    mask = bytearray()
    for i in range(0, len(nums) - 1, 2):
        mask += bytes([nums[i]]) * nums[i + 1]
    mask = Image.frombytes("L", (n, n), bytes(mask[:n * n].ljust(n * n, b"\0"))).resize((size, size), Image.LANCZOS)
    img = Image.new("RGBA", (size, size), colour + (0,))
    img.putalpha(mask)
    return ImageReader(img)


EMBLEM_RED = emblem((0x8E, 0x1B, 0x16))
EMBLEM_INK = emblem((0x6F, 0x66, 0x5A), 160)

# ---------------------------------------------------------------------------------------------- styles
BODY = ParagraphStyle("body", fontName="Mono", fontSize=8.9, leading=13.1, textColor=INK, spaceAfter=4.2, allowWidows=0, allowOrphans=0)
INDENT = ParagraphStyle("indent", parent=BODY, leftIndent=14)
BULLET = ParagraphStyle("bullet", parent=BODY, leftIndent=26, bulletIndent=12, spaceAfter=2.6)
SMALL = ParagraphStyle("small", parent=BODY, fontSize=7.8, leading=11.2, textColor=FADED)
LABEL = ParagraphStyle("label", parent=BODY, fontName="MonoB", fontSize=8.5, leading=12.2, spaceAfter=0)
CELL = ParagraphStyle("cell", parent=BODY, fontSize=8.5, leading=12.2, spaceAfter=0)
SUB = ParagraphStyle("sub", parent=BODY, fontName="Narrow", fontSize=11.5, leading=14, textColor=RED, spaceBefore=6, spaceAfter=3, keepWithNext=1)
HEAD = ParagraphStyle("head", fontName="Narrow", fontSize=16, leading=20, textColor=INK, spaceBefore=15, spaceAfter=1, keepWithNext=1)
TOC = ParagraphStyle("toc", fontName="Mono", fontSize=9, leading=13.6, textColor=INK, leftIndent=30, firstLineIndent=-30)
TOC_HEAD = ParagraphStyle("tochead", fontName="Narrow", fontSize=22, leading=26, textColor=INK, alignment=TA_CENTER)
TOC_SUB = ParagraphStyle("tocsub", fontName="Mono", fontSize=9, leading=13, textColor=FADED, alignment=TA_CENTER, spaceAfter=10)


def esc(s):
    return escape(s).replace("  ", " \u00a0")


def runin(s):
    """LIKE THIS. at the start of a paragraph is set heavier, as a typist would underline it."""
    m = re.match(r"^((?:[A-Z0-9][A-Z0-9'\u2019/&()\-]*,? ){0,8}[A-Z][A-Z0-9'\u2019/&()\-]+[.:])( |$)", s)
    if m and len(m.group(1)) > 3:
        return '<font name="MonoB">' + esc(m.group(1)) + "</font>" + esc(s[len(m.group(1)):])
    return esc(s)


# ---------------------------------------------------------------------------------------------- reading the manual
lines = open(SRC, encoding="utf-8").read().split("\n")
title, subtitle = lines[0].strip(), lines[1].strip()
sections = []                       # (number, heading, [lines])
preface, cur = [], None
i = 2
while i < len(lines):
    if lines[i].startswith("=====") and i + 2 < len(lines) and lines[i + 2].startswith("====="):
        m = re.match(r"\s*(\d+)\.\s*(.*)", lines[i + 1])
        cur = (m.group(1), m.group(2).strip(), [])
        sections.append(cur)
        i += 3
        continue
    (cur[2] if cur else preface).append(lines[i].rstrip())
    i += 1

STRICT = re.compile(r"^(\s*)(\S.*?) ?\.{3,}(?: (\S.*))?$")
LINEW = W - LM - RM
CH = pdfmetrics.stringWidth("0", "MonoB", 8.5)


def is_caps(l):
    return bool(l) and l[0] != " " and l.upper() == l and any(c.isalpha() for c in l) and len(l) < 50 and not l.endswith(".")


def flow(body):
    """One section's lines as flowables."""
    out = []
    n = 0
    while n < len(body):
        l = body[n]
        if not l.strip():
            out.append(Spacer(1, 3.4))
            n += 1
            continue
        # a run of lines of the same kind (indented or not), without a blank between them
        ind = l[0] == " "
        e = n
        while e < len(body) and body[e].strip() and (body[e][0] == " ") == ind:
            e += 1
        run = body[n:e]
        strict = [STRICT.match(x) for x in run]
        # ---- a table typed in columns (two spaces or more between them), unindented
        if not ind and sum(1 for x in run if re.search(r"\S {2,}\S", x)) >= 3 and not any(strict):
            rows = [re.split(r" {2,}", x.strip()) for x in run]
            cols = max(len(r) for r in rows)
            data = [[Paragraph(esc(c), LABEL if (k == 0 or j == 0) else CELL) for j, c in enumerate(r + [""] * (cols - len(r)))] for k, r in enumerate(rows)]
            w0 = max(len(r[0]) for r in rows) * CH + 12
            w1 = max(len(r[1]) for r in rows if len(r) > 1) * CH + 14
            t = Table(data, colWidths=[w0, w1] + [(LINEW - w0 - w1) / max(1, cols - 2)] * (cols - 2), repeatRows=1)
            t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0), ("RIGHTPADDING", (0, 0), (-1, -1), 6),
                                   ("TOPPADDING", (0, 0), (-1, -1), 2.2), ("BOTTOMPADDING", (0, 0), (-1, -1), 2.6),
                                   ("LINEBELOW", (0, 0), (-1, 0), 0.9, INK), ("LINEBELOW", (0, 1), (-1, -1), 0.3, RULE)]))
            out += [Spacer(1, 3), t, Spacer(1, 5)]
            n = e
            continue
        # ---- "label ...... text" lines: a list in two columns
        if sum(1 for m in strict if m) >= 1:
            cols = Counter(m.start(3) for m in strict if m and m.group(3))
            col = cols.most_common(1)[0][0] if cols else 0
            base = min(len(x) - len(x.lstrip()) for x in run)
            rows, pending = [], []

            def flush():
                nonlocal rows
                if not rows:
                    return
                lw = min(max(len(a) for a, _ in rows) * CH + 12, LINEW * 0.47)
                left = 12 if ind else 0
                data = [["", Paragraph(esc(a).replace(" ", "\u00a0") if a.startswith(" ") else esc(a), LABEL), Paragraph(esc(b), CELL)] for a, b in rows]
                t = Table(data, colWidths=[left, lw, LINEW - lw - left])
                t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0), ("RIGHTPADDING", (0, 0), (-1, -1), 6),
                                       ("TOPPADDING", (0, 0), (-1, -1), 1.6), ("BOTTOMPADDING", (0, 0), (-1, -1), 2.4)]))
                out.append(t)
                out.append(Spacer(1, 3.5))
                rows = []

            for x, m in zip(run, strict):
                if m:
                    rows.append((x[base:m.end(2)].rstrip(), m.group(3) or ""))
                elif col and len(x) > col and x[col] != " " and x[col - 1] == " " and x[col - 2] in " ." and ". " not in x[:col - 1].rstrip(" .") and ": " not in x[:col]:
                    rows.append((x[base:col].rstrip(" ."), x[col:]))
                    SPLITS.append(x)
                else:
                    flush()
                    out.extend(flow([x]))
            flush()
            n = e
            continue
        # ---- everything else, a line at a time
        for x in run:
            s = x.strip()
            mb = re.match(r"^(-|\d+\.) +(.*)$", s)
            if ind and mb:
                out.append(Paragraph(runin(mb.group(2)), BULLET, bulletText="\u2013" if mb.group(1) == "-" else mb.group(1)))
            elif ind:
                out.append(Paragraph(esc(re.sub(r" {3,}", "     ", s)), INDENT))
            elif is_caps(s):
                out.append(Paragraph(esc(s), SUB))
            else:
                out.append(Paragraph(runin(s), BODY))
        n = e
    return out


SPLITS = []


class Heading(Paragraph):
    """A section heading: it enters itself in the inventory and in the PDF's outline."""
    def __init__(self, num, text):
        Paragraph.__init__(self, f'<font color="#A9231C" size="21">{num}</font><font size="8"> \u00a0</font>{esc(text)}', HEAD)
        self.num, self.plain = num, text


# ---------------------------------------------------------------------------------------------- the pages
TOTAL = [0]


def stamp(c, x, y, lines, colour, angle=0, size=11, pad=5, alpha=0.82, font="Narrow"):
    c.saveState()
    c.translate(x, y)
    c.rotate(angle)
    w = max(pdfmetrics.stringWidth(t, font, size) for t in lines) + 2 * pad + 4
    h = len(lines) * size * 1.22 + 2 * pad
    c.setStrokeColor(colour); c.setFillColor(colour)
    c.setStrokeAlpha(alpha); c.setFillAlpha(alpha)
    c.setLineWidth(1.5); c.rect(-w / 2, -h / 2, w, h)
    c.setLineWidth(0.5); c.rect(-w / 2 + 2.4, -h / 2 + 2.4, w - 4.8, h - 4.8)
    c.setFont(font, size)
    for k, t in enumerate(lines):
        c.drawCentredString(0, h / 2 - pad - size * 0.92 - k * size * 1.22, t)
    c.restoreState()


def holes(c, colour):
    c.setFillColor(colour); c.setStrokeColor(HexColor("#8E8166")); c.setLineWidth(0.4)
    for y in (H / 2 + 40 * mm, H / 2 - 40 * mm):
        c.circle(11 * mm, y, 2.6 * mm, stroke=1, fill=1)


def cover(c, doc):
    c.saveState()
    c.setFillColor(MANILA); c.rect(0, 0, W, H, stroke=0, fill=1)
    c.setFillColor(MANILA_D); c.rect(0, 0, 21 * mm, H, stroke=0, fill=1)           # the spine
    c.setStrokeColor(HexColor("#9C8048")); c.setLineWidth(0.6); c.setDash(4, 3)
    c.line(21 * mm, 0, 21 * mm, H); c.setDash()
    holes(c, HexColor("#7A6A48"))
    cx = (W + 21 * mm) / 2
    # printed heading of the folder
    c.setFillColor(HexColor("#4A3A22"))
    c.setFont("Narrow", 13); c.drawCentredString(cx, H - 31 * mm, "КОМИТЕТ ГОСУДАРСТВЕННОЙ БЕЗОПАСНОСТИ")
    c.setFont("Narrow", 10.5); c.drawCentredString(cx, H - 36.5 * mm, "ПРИ СОВЕТЕ МИНИСТРОВ СССР")
    c.setFont("Narrow", 11.5); c.drawCentredString(cx, H - 44.5 * mm, "ПЕРВОЕ ГЛАВНОЕ УПРАВЛЕНИЕ")
    c.drawImage(EMBLEM_RED, cx - 23 * mm, H - 97 * mm, 46 * mm, 46 * mm, mask="auto")
    c.setFont("Display", 30); c.setFillColor(HexColor("#3A2C18"))
    dw = pdfmetrics.stringWidth("ДЕЛО №", "Display", 30)
    x0 = cx - (dw + 42 * mm) / 2
    c.drawString(x0, H - 117 * mm, "ДЕЛО №")
    c.setStrokeColor(HexColor("#4A3A22")); c.setLineWidth(0.8)
    c.line(x0 + dw + 5 * mm, H - 118 * mm, x0 + dw + 42 * mm, H - 118 * mm)
    c.setFont("Hand", 22); c.setFillColor(PENCIL); c.drawString(x0 + dw + 9 * mm, H - 116.4 * mm, "68/91-Р")
    # the label gummed to the folder
    lx, ly, lw, lh = cx - 72 * mm, H - 214 * mm, 144 * mm, 82 * mm
    c.setFillColor(Color(0, 0, 0, 0.16)); c.rect(lx + 1.6, ly - 1.8, lw, lh, stroke=0, fill=1)
    c.setFillColor(PAPER); c.setStrokeColor(HexColor("#8E8166")); c.setLineWidth(0.5); c.rect(lx, ly, lw, lh, stroke=1, fill=1)
    c.setLineWidth(1.1); c.setStrokeColor(INK); c.rect(lx + 3 * mm, ly + 3 * mm, lw - 6 * mm, lh - 6 * mm)
    c.setLineWidth(0.4); c.rect(lx + 4 * mm, ly + 4 * mm, lw - 8 * mm, lh - 8 * mm)
    c.setFillColor(INK)
    c.setFont("Display", 25); c.drawCentredString(cx, ly + lh - 23 * mm, "REZIDENTURA")
    c.setFont("Narrow", 14.5); c.setFillColor(RED); c.drawCentredString(cx, ly + lh - 34 * mm, "НАСТАВЛЕНИЕ ОПЕРАТИВНОМУ РАБОТНИКУ")
    c.setFillColor(INK); c.setFont("MonoM", 12.5); c.drawCentredString(cx, ly + lh - 43.5 * mm, "OFFICER'S HANDBOOK")
    c.setFont("Mono", 9.5); c.drawCentredString(cx, ly + lh - 50 * mm, subtitle)
    c.setFont("Mono", 9); c.setFillColor(INK)
    rows = [("Начато", "июнь 1968 г.", "Begun"), ("Окончено", "август 1991 г.", "Closed"), ("На", (str(TOTAL[0]) if TOTAL[0] else "  ") + " листах", "Sheets")]
    for k, (ru, val, en) in enumerate(rows):
        x0 = lx + 12 * mm + k * 42 * mm
        c.setFont("Mono", 8.4); c.setFillColor(INK); c.drawString(x0, ly + 16.5 * mm, ru)
        c.setFont("Hand", 12.5); c.setFillColor(PENCIL); c.drawString(x0 + pdfmetrics.stringWidth(ru + " ", "Mono", 8.4) + 2, ly + 16.5 * mm, val)
        c.setStrokeColor(FADED); c.setLineWidth(0.3); c.line(x0, ly + 15.2 * mm, x0 + 38 * mm, ly + 15.2 * mm)
        c.setFont("Mono", 6.6); c.setFillColor(FADED); c.drawString(x0, ly + 12 * mm, en)
    # stamps
    stamp(c, W - 52 * mm, H - 64 * mm, ["СОВЕРШЕННО", "СЕКРЕТНО"], RED, angle=-8, size=15, pad=6)
    stamp(c, cx - 42 * mm, 58 * mm, ["ХРАНИТЬ ПОСТОЯННО"], VIOLET, angle=5, size=12, pad=5, alpha=0.7)
    c.setFillColor(HexColor("#4A3A22")); c.setFont("Mono", 9); c.drawRightString(W - 19 * mm, H - 19 * mm, "Экз. № 1")
    c.setFont("Hand", 15); c.setFillColor(PENCIL)
    c.saveState(); c.translate(cx + 14 * mm, 55 * mm); c.rotate(4); c.drawString(0, 0, "Ознакомиться до выезда."); c.restoreState()
    c.setFont("Narrow", 9.5); c.setFillColor(HexColor("#4A3A22"))
    c.drawCentredString(cx, 34 * mm, "ВЫНОСИТЬ ИЗ ПОМЕЩЕНИЯ РЕЗИДЕНТУРЫ ЗАПРЕЩАЕТСЯ")
    c.setFont("Mono", 7); c.setFillColor(HexColor("#5E4D2E"))
    c.drawCentredString(cx, 22 * mm, f"The handbook of the game Rezidentura, version {VERSION}. Every person, operation and paper in it is invented.")
    c.drawCentredString(cx, 18 * mm, "Stamps: top secret - copy no. 1 - keep permanently - read before departure - not to leave the station's premises.")
    c.restoreState()


def page(c, doc):
    c.saveState()
    c.setFillColor(PAPER); c.rect(0, 0, W, H, stroke=0, fill=1)
    holes(c, HexColor("#D9CFB6"))
    n = c.getPageNumber()
    c.setFont("Narrow", 8.5); c.setFillColor(FADED)
    c.drawString(LM, H - 15.5 * mm, "ДЕЛО № 68/91-Р")
    c.setFont("Mono", 7.4); c.drawString(LM + 27 * mm, H - 15.5 * mm, "REZIDENTURA - OFFICER'S HANDBOOK")
    c.setFillColor(RED); c.setFont("Narrow", 8.5); c.drawRightString(W - RM - 17 * mm, H - 15.5 * mm, "СОВ. СЕКРЕТНО")
    c.setStrokeColor(RULE); c.setLineWidth(0.5); c.line(LM, H - 17.5 * mm, W - RM, H - 17.5 * mm)
    # the sheet number, pencilled in the corner as the registry does
    c.setFont("Hand", 15); c.setFillColor(PENCIL); c.drawRightString(W - RM - 1.5, H - 15.0 * mm, str(n))
    c.setFont("Mono", 7.6); c.setFillColor(FADED); c.drawCentredString((LM + W - RM) / 2, 13 * mm, f"- {n} -")
    c.restoreState()
    TOTAL[0] = max(TOTAL[0], n)


class Doc(BaseDocTemplate):
    def afterFlowable(self, f):
        if isinstance(f, Heading):
            key = "sec" + f.num
            self.canv.bookmarkPage(key)
            self.canv.addOutlineEntry(f"{f.num}. {f.plain}", key, 0, False)
            self.notify("TOCEntry", (0, f"{f.num:>2}.\u00a0\u00a0{esc(f.plain)}", self.page, key))


doc = Doc(OUT, pagesize=A4, leftMargin=LM, rightMargin=RM, topMargin=TM, bottomMargin=BM,
          title="Rezidentura - Officer's Handbook", author="Rezidentura", subject="The handbook of the game Rezidentura, version " + VERSION)
frame = Frame(LM, BM, W - LM - RM, H - TM - BM, leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0, id="f")
doc.addPageTemplates([PageTemplate("cover", [frame], onPage=cover), PageTemplate("body", [frame], onPage=page)])

story = [NextPageTemplate("body"), PageBreak()]
# ---- the inventory
toc = TableOfContents()
toc.levelStyles = [TOC]
toc.dotsMinLevel = 0
toc.tableStyle = TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0), ("RIGHTPADDING", (0, 0), (-1, -1), 0),
                             ("TOPPADDING", (0, 0), (-1, -1), 0.4), ("BOTTOMPADDING", (0, 0), (-1, -1), 0.4)])
story += [Spacer(1, 4), Paragraph("ВНУТРЕННЯЯ ОПИСЬ", TOC_HEAD), Paragraph("inventory of the papers in this file", TOC_SUB),
          HRFlowable(width="100%", thickness=1.1, color=INK, spaceAfter=1.5), HRFlowable(width="100%", thickness=0.4, color=INK, spaceAfter=8), toc, PageBreak()]
# ---- the note at the head of the manual
pre = [x for x in preface if x.strip()]
if pre:
    story += [Paragraph("К СВЕДЕНИЮ", SUB)] + [Paragraph(esc(x), BODY) for x in pre] + [Spacer(1, 4)]
for num, head, body in sections:
    while body and not body[0].strip(): body.pop(0)
    while body and not body[-1].strip(): body.pop()
    h1 = HRFlowable(width="100%", thickness=1.1, color=INK, spaceBefore=1, spaceAfter=1.5)
    h2 = HRFlowable(width="100%", thickness=0.4, color=INK, spaceAfter=7)
    h1.keepWithNext = h2.keepWithNext = True
    story += [Heading(num, head), h1, h2] + flow(body)
# ---- the last sheet: who has read the file
story += [PageBreak(), Spacer(1, 6), Paragraph("ЛИСТ ОЗНАКОМЛЕНИЯ", TOC_HEAD), Paragraph("the file has been read by", TOC_SUB)]
head = [Paragraph(f'<font name="Narrow" size="9.5">{a}</font><br/><font size="7" color="#6F665A">{b}</font>', CELL)
        for a, b in [("ДАТА", "date"), ("ДОЛЖНОСТЬ, ЗВАНИЕ, ФАМИЛИЯ", "post, rank, name"), ("ПОДПИСЬ", "signature")]]
rows = [head] + [["", "", ""] for _ in range(18)]
t = Table(rows, colWidths=[LINEW * 0.2, LINEW * 0.52, LINEW * 0.28], rowHeights=[30] + [28] * 18)
t.setStyle(TableStyle([("GRID", (0, 0), (-1, -1), 0.5, FADED), ("BOX", (0, 0), (-1, -1), 1.1, INK), ("LINEBELOW", (0, 0), (-1, 0), 1.1, INK),
                       ("VALIGN", (0, 0), (-1, -1), "MIDDLE"), ("LEFTPADDING", (0, 0), (-1, -1), 6)]))
story += [t, Spacer(1, 10), Paragraph("Pencil only. The file is returned to the registry the same day, against signature.", SMALL)]

# Two passes at least: the inventory needs the page numbers, and the cover the number of sheets.
doc.multiBuild(story)
print(f"{OUT}: {TOTAL[0]} sheets; {len(sections)} sections; {len(SPLITS)} list lines split by column")
if "-v" in sys.argv:
    for x in SPLITS: print("   ", x[:110])
