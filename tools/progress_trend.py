#!/usr/bin/env python3
"""Trend graphic of the README progress table, drawn from the git history of README.md.

Each README version that has a readable progress table is one point, keyed by its "Updated <date>, commit <hash>." line.
Called by tools/progress_report.py; the SVG is written to docs/progress-trend.svg.
"""
import datetime
import re
import subprocess
from pathlib import Path
from xml.sax.saxutils import escape

START_MARKER = '<!-- progress:start -->'
END_MARKER = '<!-- progress:end -->'
TREND_PATH = Path('docs') / 'progress-trend.svg'
TREND_IMAGE = '![Progress trend](docs/progress-trend.svg)'
WIDTH, HEIGHT = 720, 200
PANEL_WIDTH = WIDTH // 4
PLOT_TOP, PLOT_BOTTOM = 68, 158
PANEL_PADDING = 14
LABEL_GAP = 12
VALUE_FIELDS = ('script', 'script_max', 'objects', 'objects_max', 'open', 'stand_ins', 'code', 'code_text')
UPDATED = re.compile(r'Updated (\d{4}-\d{2}-\d{2}), commit ([^.\s]+)\.')
COUNT = re.compile(r'(\d+) of (\d+) \(')
OPEN_WORK = re.compile(r'(\d+) open items, (\d+) stand-ins$')
CODE = re.compile(r'(\d+) of about (\d+) \((\d+\.\d+)%\)$')
STYLE = """<style>
text{font-family:system-ui,sans-serif}
.title{font-size:13px;font-weight:600}
.big{font-size:22px;font-weight:600}
.label{font-size:10px}
.date{font-size:10px}
.surface{fill:#fcfcfb}
.ink{fill:#0b0b0b}
.muted{fill:#52514e}
.rule{stroke:#c9c8c3;stroke-width:1}
.line1{fill:none;stroke:#2a78d6;stroke-width:2;stroke-linejoin:round;stroke-linecap:round}
.dot1{fill:#2a78d6}
.line2{fill:none;stroke:#eb6834;stroke-width:2;stroke-linejoin:round;stroke-linecap:round}
.dot2{fill:#eb6834}
@media (prefers-color-scheme: dark) {
.surface{fill:#1a1a19}
.ink{fill:#f0efec}
.muted{fill:#c3c2b7}
.rule{stroke:#4a4a46}
.line1{stroke:#3987e5}
.dot1{fill:#3987e5}
.line2{stroke:#d95926}
.dot2{fill:#d95926}
}
</style>"""


def section_text(readme):
    start, end = readme.find(START_MARKER), readme.find(END_MARKER)
    if start < 0 or end < start:
        return None
    return readme[start + len(START_MARKER):end].strip()


def parse_section(section):
    """One point from a progress section, or None when its table or its Updated line cannot be read."""
    updated = UPDATED.search(section)
    cells = {}
    for line in section.splitlines():
        if line.startswith('|'):
            parts = [part.strip() for part in line.strip().strip('|').split('|')]
            if len(parts) >= 2:
                cells.setdefault(parts[0], parts[1])
    script = COUNT.match(cells.get('Script commands', ''))
    objects = COUNT.match(cells.get('Object types', ''))
    open_work = OPEN_WORK.match(cells.get('Open work', ''))
    code = CODE.match(cells.get('Game code documented', ''))
    if not (updated and script and objects and open_work and code):
        return None
    return {
        'date': updated.group(1),
        'stamp': 'Updated %s, commit %s' % (updated.group(1), updated.group(2)),
        'script': int(script.group(1)),
        'script_max': int(script.group(2)),
        'objects': int(objects.group(1)),
        'objects_max': int(objects.group(2)),
        'open': int(open_work.group(1)),
        'stand_ins': int(open_work.group(2)),
        'code': float(code.group(3)),
        'code_text': '%s%%' % code.group(3),
    }


def _values(point):
    return tuple(point[field] for field in VALUE_FIELDS)


def build_points(sections, current):
    """Points oldest first: one per stamp, with the current section included, and a run of equal values kept once.

    The oldest point of a run is kept, so the graphic is the same before and after the commit that records the current section.
    """
    points = {}
    for section in [*sections, current]:
        point = parse_section(section)
        if point and point['stamp'] not in points:
            points[point['stamp']] = point
    kept = []
    for point in sorted(points.values(), key=lambda point: point['date']):
        if not kept or _values(kept[-1]) != _values(point):
            kept.append(point)
    return kept


def _git(root, *args):
    result = subprocess.run(['git', '-C', str(root), *args], capture_output=True, text=True, check=False)
    return result.stdout if result.returncode == 0 else ''


def history_sections(root):
    """Progress sections of every README version on the first-parent history of HEAD, oldest first."""
    commits = _git(root, 'rev-list', '--first-parent', '--reverse', 'HEAD', '--', 'README.md').split()
    sections = []
    for commit in commits:
        section = section_text(_git(root, 'show', '%s:README.md' % commit))
        if section is not None:
            sections.append(section)
    return sections


MONTHS = ('Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec')


def _short_date(date):
    parsed = datetime.date.fromisoformat(date)
    return '%d %s' % (parsed.day, MONTHS[parsed.month - 1])


def _scale(values, zero_based):
    low = 0 if zero_based else min(values)
    high = max(values)
    if high == low:
        return (0, 1) if zero_based else (low - 1, high + 1)
    return low, high


def _number(value):
    return '%.1f' % value


def _panel(left, plot_width, title, big, series, positions, zero_based, end_labels, first_date, last_date):
    """SVG parts of one panel. series: [{'points': [(position, value)], 'css', 'dot'}]; end_labels: [(value, text, css)]."""
    values = [value for line in series for _position, value in line['points']]
    low, high = _scale(values, zero_based)
    first, last = min(positions), max(positions)

    def x_of(position):
        if last > first:
            return left + (position - first) / (last - first) * plot_width
        return left + plot_width / 2

    def y_of(value):
        return PLOT_TOP + (high - value) / (high - low) * (PLOT_BOTTOM - PLOT_TOP)

    parts = ['<text class="title ink" x="%d" y="22">%s</text>' % (left, escape(title)),
             '<text class="big ink" x="%d" y="50">%s</text>' % (left, escape(big))]
    for line in series:
        coords = [(_number(x_of(position)), _number(y_of(value))) for position, value in line['points']]
        parts.append('<polyline class="%s" points="%s"/>' % (line['css'], ' '.join('%s,%s' % xy for xy in coords)))
        for x, y in coords:
            parts.append('<circle class="%s" cx="%s" cy="%s" r="2.5"/>' % (line['dot'], x, y))
    parts.append('<line class="rule" x1="%d" y1="%d" x2="%d" y2="%d"/>' % (left, PLOT_BOTTOM, left + plot_width, PLOT_BOTTOM))
    parts.append('<text class="date muted" x="%d" y="178">%s</text>' % (left, escape(first_date)))
    if last_date != first_date:
        parts.append('<text class="date muted" x="%d" y="178" text-anchor="end">%s</text>' % (left + plot_width, escape(last_date)))
    spaced = []
    for y, text, css in sorted((y_of(value), text, css) for value, text, css in end_labels):
        if spaced and y - spaced[-1][0] < LABEL_GAP:
            y = spaced[-1][0] + LABEL_GAP
        spaced.append((y, text, css))
    for y, text, css in spaced:
        parts.append('<text class="label %s" x="%d" y="%s">%s</text>' % (css, left + plot_width + 6, _number(y + 3.5), escape(text)))
    return parts


def trend_svg(points):
    if not points:
        raise ValueError('no progress points for the trend graphic')
    positions = list(range(len(points)))
    first_date, last_date = _short_date(points[0]['date']), _short_date(points[-1]['date'])
    latest = points[-1]
    plot_width = PANEL_WIDTH - 2 * PANEL_PADDING
    lefts = [index * PANEL_WIDTH + PANEL_PADDING for index in range(4)]

    def line(key, css, dot):
        return {'points': [(position, point[key]) for position, point in zip(positions, points)], 'css': css, 'dot': dot}

    script = _panel(lefts[0], plot_width, 'Script commands', '%d of %d' % (latest['script'], latest['script_max']),
                    [line('script', 'line1', 'dot1')], positions, True, [], first_date, last_date)
    objects = _panel(lefts[1], plot_width, 'Object types', '%d of %d' % (latest['objects'], latest['objects_max']),
                     [line('objects', 'line1', 'dot1')], positions, True, [], first_date, last_date)
    open_work = _panel(lefts[2], plot_width - 62, 'Open work', '%d / %d' % (latest['open'], latest['stand_ins']),
                       [line('open', 'line1', 'dot1'), line('stand_ins', 'line2', 'dot2')], positions, True,
                       [(latest['open'], 'open %d' % latest['open'], 'ink'),
                        (latest['stand_ins'], 'stand-ins %d' % latest['stand_ins'], 'ink')],
                       first_date, last_date)
    code = _panel(lefts[3], plot_width, 'Game code documented', latest['code_text'],
                  [line('code', 'line1', 'dot1')], positions, False, [], first_date, last_date)
    rules = ['<line class="rule" x1="%d" y1="12" x2="%d" y2="190"/>' % (index * PANEL_WIDTH, index * PANEL_WIDTH) for index in (1, 2, 3)]
    body = script + objects + open_work + code + rules
    return '\n'.join(
        ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d" role="img">' % (WIDTH, HEIGHT, WIDTH, HEIGHT),
         '<title>Progress of the PC port over time: script commands, object types, open work and game code documented</title>',
         STYLE,
         '<rect class="surface" width="%d" height="%d"/>' % (WIDTH, HEIGHT)]
        + body + ['</svg>']) + '\n'


def trend_svg_for(root, current):
    return trend_svg(build_points(history_sections(root), current))


def write_trend(root, current):
    path = root / TREND_PATH
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(trend_svg_for(root, current).encode('utf-8'))
    return path


def trend_is_current(root, current):
    path = root / TREND_PATH
    return path.exists() and path.read_bytes() == trend_svg_for(root, current).encode('utf-8')
