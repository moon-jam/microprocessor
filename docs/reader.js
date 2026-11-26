const findForm = document.getElementById("page-find");
const findButton = document.getElementById("open-find");
const findInput = document.getElementById("find-input");
const findStatus = document.getElementById("find-status");
const shortcutDialog = document.getElementById("shortcuts");
const shortcutButton = document.getElementById("open-shortcuts");
const enableVim = document.getElementById("enable-vim");
const searchBlocks = [...document.querySelectorAll(
    "main :is(h1, h2, h3, h4, p, li, th, td, figcaption, summary, pre)"
)];

const exampleDetails = [...document.querySelectorAll("main details")];
const journal = performance.timeOrigin;
const places = [];
let placeIndex = 0;
let matches = [];
let matchIndex = -1;
let lastG = -Infinity;

function savePlace() {
    places[placeIndex] = {
        y: window.scrollY,
        open: exampleDetails.map(details => details.open),
    };
}

history.replaceState({ journal, placeIndex }, "");
history.scrollRestoration = "manual";
savePlace();

function jumpTo(move, url = location.href) {
    savePlace();
    const next = placeIndex + 1;
    history.pushState({ journal, placeIndex: next }, "", url);
    places.length = next + 1;
    placeIndex = next;
    move();
    savePlace();
}

window.addEventListener("scroll", savePlace, { passive: true });
document.addEventListener("toggle", event => {
    if (exampleDetails.includes(event.target)) savePlace();
}, true);
window.addEventListener("popstate", event => {
    if (event.state?.journal !== journal) return;
    placeIndex = event.state.placeIndex;
    const place = places[placeIndex];
    exampleDetails.forEach((details, index) => { details.open = place.open[index]; });
    window.scrollTo(0, place.y);
});

document.addEventListener("click", event => {
    const anchor = event.target.closest("a[href^='#']");
    if (!anchor || event.defaultPrevented || event.button !== 0 ||
        event.altKey || event.metaKey || event.ctrlKey || event.shiftKey) return;

    const target = document.getElementById(anchor.hash.slice(1));
    if (!target) return;
    event.preventDefault();
    jumpTo(() => {
        target.tabIndex = -1;
        target.focus({ preventScroll: true });
        target.scrollIntoView();
    }, anchor.href);
});

function openFind() {
    findForm.hidden = false;
    findButton.setAttribute("aria-expanded", "true");
    findInput.focus({ preventScroll: true });
    findInput.select();
}

function closeFind() {
    if (findForm.contains(document.activeElement)) findButton.focus({ preventScroll: true });
    findForm.hidden = true;
    findButton.setAttribute("aria-expanded", "false");
    CSS.highlights?.delete("docs-find");
    CSS.highlights?.delete("docs-find-current");
}

function findRanges(block, pattern) {
    const hits = [...block.textContent.matchAll(pattern)];
    if (!hits.length) return [];

    const walker = document.createTreeWalker(block, NodeFilter.SHOW_TEXT);
    const nodes = [];
    let offset = 0;
    while (walker.nextNode()) {
        const node = walker.currentNode;
        nodes.push({ node, start: offset, end: offset + node.length });
        offset += node.length;
    }
    return hits.map(hit => {
        const end = hit.index + hit[0].length;
        const first = nodes.find(node => node.end > hit.index);
        const last = nodes.find(node => node.end >= end);
        const range = new Range();
        range.setStart(first.node, hit.index - first.start);
        range.setEnd(last.node, end - last.start);
        return { block, range };
    });
}

function showMatches() {
    if (!CSS.highlights) return;
    CSS.highlights.set("docs-find", new Highlight(...matches.map(match => match.range)));
    CSS.highlights.delete("docs-find-current");
    if (matchIndex >= 0) {
        const active = new Highlight(matches[matchIndex].range);
        active.priority = 1;
        CSS.highlights.set("docs-find-current", active);
    }
}

function findText() {
    const query = findInput.value.trim();
    const literal = query.replace(/[.*+?^$(){}|[\]\\]/g, "\\$&");
    const pattern = new RegExp(literal, "giu");
    matches = query ? searchBlocks.flatMap(block => findRanges(block, pattern)) : [];
    matchIndex = -1;
    findStatus.value = query ? matches.length + " 筆" : "";
    showMatches();
}

function matchTop({ block, range }) {
    return (block.closest("pre")?.closest("details:not([open])") ?? range)
        .getBoundingClientRect().top;
}

function moveMatch(direction) {
    if (!matches.length) return false;
    if (matchIndex === -1) {
        const top = parseFloat(getComputedStyle(root).scrollPaddingTop);
        matchIndex = direction > 0
            ? matches.findIndex(match => matchTop(match) >= top)
            : matches.findLastIndex(match => matchTop(match) < top);
        if (matchIndex === -1) matchIndex = direction > 0 ? 0 : matches.length - 1;
    } else {
        matchIndex = (matchIndex + direction + matches.length) % matches.length;
    }
    const { block, range } = matches[matchIndex];

    jumpTo(() => {
        const details = block.closest("pre")?.closest("details");
        if (details) details.open = true;
        const pre = block.closest("pre");
        if (pre) {
            const match = range.getBoundingClientRect();
            const view = pre.getBoundingClientRect();
            if (match.left < view.left || match.right > view.right) {
                pre.scrollLeft += match.left - view.left - 16;
            }
        }
        window.scrollBy(0, range.getBoundingClientRect().top - window.innerHeight / 2);
    });
    showMatches();
    findStatus.value = (matchIndex + 1) + " / " + matches.length;
    return true;
}

findButton.hidden = shortcutButton.hidden = false;
findButton.addEventListener("click", openFind);
shortcutButton.addEventListener("click", () => shortcutDialog.showModal());
document.getElementById("close-find").addEventListener("click", closeFind);
document.getElementById("find-previous").addEventListener("click", () => moveMatch(-1));
findInput.addEventListener("input", findText);
findForm.addEventListener("submit", event => {
    event.preventDefault();
    if (moveMatch(1)) findInput.blur();
});

document.addEventListener("keydown", event => {
    if (event.defaultPrevented || event.isComposing || shortcutDialog.open) return;
    if (!findForm.hidden && event.key === "Escape") {
        event.preventDefault();
        closeFind();
        return;
    }
    if (event.target === findInput && event.key === "Enter" && event.shiftKey) {
        event.preventDefault();
        if (moveMatch(-1)) findInput.blur();
        return;
    }
    if (!enableVim.checked || event.altKey || event.metaKey ||
        event.target.closest("input, textarea, select, [contenteditable]:not([contenteditable='false'])")) return;

    const key = event.key;
    if (key !== "g") lastG = -Infinity;
    if (event.ctrlKey) {
        if (event.shiftKey) return;
        if (key === "o" || key === "i") {
            event.preventDefault();
            const step = key === "o" ? -1 : 1;
            if (!event.repeat && history.state?.journal === journal && places[placeIndex + step]) {
                savePlace();
                history.go(step);
            }
        } else if (key === "d" || key === "u") {
            event.preventDefault();
            window.scrollBy(0, window.innerHeight / (key === "d" ? 2 : -2));
        }
        return;
    }

    if (key === "j" || key === "k") {
        event.preventDefault();
        window.scrollBy(0, key === "j" ? 60 : -60);
        return;
    }
    if (event.repeat) return;
    switch (key) {
        case "/":
            event.preventDefault();
            openFind();
            break;
        case "?":
            event.preventDefault();
            shortcutDialog.showModal();
            break;
        case "n":
        case "N":
            if (moveMatch(key === "n" ? 1 : -1)) event.preventDefault();
            break;
        case "g":
            if (event.timeStamp - lastG < 750) {
                jumpTo(() => window.scrollTo(0, 0));
                lastG = -Infinity;
            } else {
                lastG = event.timeStamp;
            }
            break;
        case "G":
            jumpTo(() => window.scrollTo(0, root.scrollHeight));
            break;
    }
});
