const nav = document.querySelector("nav");
const root = document.documentElement;
const header = document.querySelector("header");
const compact = matchMedia("(max-width: 52rem)");
const groups = new Map(Array.from(nav.querySelectorAll(".nav-group"), group => {
    const parent = group.querySelector(".nav-row a");
    const children = group.querySelector(".subnav");
    const toggle = document.createElement("button");
    toggle.type = "button";
    toggle.className = "nav-toggle";
    toggle.setAttribute("aria-label", parent.textContent + " subsections");
    toggle.setAttribute("aria-controls", children.id);
    toggle.setAttribute("aria-expanded", "false");
    const disclosure = { parent, children, toggle };
    toggle.addEventListener("click", () => {
        disclosure.openByUser = children.hidden;
        updateGroups();
        markCurrent();
    });
    parent.after(toggle);
    children.hidden = true;
    return [group, disclosure];
}));

const sections = Array.from(nav.querySelectorAll("a[href^='#']"), link => ({
    link,
    heading: document.getElementById(link.hash.slice(1)),
    group: link.closest(".nav-group"),
}));

let section;
let currentLink;

function updateGroups() {
    for (const [group, { children, toggle, openByUser }] of groups) {
        const open = openByUser ?? (!compact.matches && section.group === group);
        children.hidden = !open;
        toggle.setAttribute("aria-expanded", String(open));
    }
}

function markCurrent(reveal = false) {
    const group = groups.get(section.group);
    const active = group?.children.hidden ? group.parent : section.link;
    if (active === currentLink) return;
    currentLink?.removeAttribute("aria-current");
    active.setAttribute("aria-current", "location");
    currentLink = active;

    if (reveal && !compact.matches) {
        const link = active.getBoundingClientRect();
        const viewport = nav.getBoundingClientRect();
        if (link.top < viewport.top) nav.scrollTop += Math.floor(link.top - viewport.top);
        else if (link.bottom > viewport.bottom) nav.scrollTop += Math.ceil(link.bottom - viewport.bottom);
    }
}

function updateCurrentSection() {
    const threshold = parseFloat(getComputedStyle(root).scrollPaddingTop) + 1;
    const atEnd = Math.ceil(window.scrollY + window.innerHeight) >= root.scrollHeight;
    const active = (atEnd ? sections.at(-1) : sections.findLast(
        ({ heading }) => heading.getBoundingClientRect().top <= threshold
    )) ?? sections[0];
    if (active === section) return;

    section = active;
    updateGroups();
    markCurrent(true);
}

function updateHeaderHeight() {
    root.style.setProperty("--header-height", header.getBoundingClientRect().height + "px");
    updateCurrentSection();
}

compact.addEventListener("change", () => {
    updateGroups();
    markCurrent(true);
});
new ResizeObserver(updateHeaderHeight).observe(header);
window.addEventListener("scroll", updateCurrentSection, { passive: true });
window.addEventListener("resize", updateCurrentSection);
window.addEventListener("pageshow", updateCurrentSection);
updateHeaderHeight();

const sidebar = nav.parentElement;
const navResize = sidebar.querySelector(".nav-resize");
let dragStart;

function setSidebarWidth(width) {
    if (compact.matches) return;
    const style = getComputedStyle(sidebar);
    const min = parseFloat(style.minWidth);
    const max = parseFloat(style.maxWidth);
    const widthPx = Math.round(Math.max(min, Math.min(max, width)));
    if (widthPx !== sidebar.offsetWidth) {
        sidebar.style.setProperty("--nav-width", widthPx + "px");
        updateCurrentSection();
    }
    navResize.setAttribute("aria-valuemin", min);
    navResize.setAttribute("aria-valuemax", max);
    navResize.setAttribute("aria-valuenow", widthPx);
}

navResize.addEventListener("pointerdown", event => {
    if (!event.isPrimary || event.button !== 0) return;
    event.preventDefault();
    dragStart = { x: event.clientX, width: sidebar.offsetWidth };
    navResize.setPointerCapture(event.pointerId);
});
navResize.addEventListener("pointermove", event => {
    if (!navResize.hasPointerCapture(event.pointerId)) return;
    setSidebarWidth(dragStart.width + event.clientX - dragStart.x);
});
navResize.addEventListener("keydown", event => {
    if (event.key !== "ArrowLeft" && event.key !== "ArrowRight") return;
    event.preventDefault();
    setSidebarWidth(sidebar.offsetWidth + (event.key === "ArrowLeft" ? -16 : 16));
});
function syncSidebarWidth() {
    setSidebarWidth(sidebar.offsetWidth);
}
compact.addEventListener("change", syncSidebarWidth);
syncSidebarWidth();
navResize.hidden = false;

const copyStatus = document.createElement("div");
copyStatus.className = "copy-status";
copyStatus.setAttribute("role", "status");
document.querySelector("main").prepend(copyStatus);

for (const code of document.querySelectorAll("main pre > code")) {
    const pre = code.parentElement;
    const frame = document.createElement("div");
    const button = document.createElement("button");
    frame.className = "code-frame";
    button.className = "copy-code";
    button.type = "button";
    button.setAttribute("aria-label", "複製程式碼");
    button.title = "複製程式碼";
    pre.before(frame);
    frame.append(button, pre);

    let resetStatus;
    button.addEventListener("click", async () => {
        const text = code.textContent;
        let copied = false;
        try {
            if (navigator.clipboard) {
                await navigator.clipboard.writeText(text);
                copied = true;
            }
        } catch {}

        if (!copied) {
            const input = document.createElement("textarea");
            const previousFocus = document.activeElement;
            input.value = text;
            input.readOnly = true;
            input.style.cssText = "position:fixed;top:0;left:0;opacity:0";
            document.body.append(input);
            input.focus({ preventScroll: true });
            input.select();
            try {
                copied = document.execCommand("copy");
            } catch {}
            input.remove();
            previousFocus?.focus({ preventScroll: true });
        }

        if (!copied) {
            const range = document.createRange();
            const selection = window.getSelection();
            range.selectNodeContents(code);
            selection.removeAllRanges();
            selection.addRange(range);
        }

        button.dataset.state = copied ? "copied" : "selected";
        button.title = copied ? "已複製" : "已選取程式碼，請按 ⌘C 或 Ctrl+C";
        copyStatus.textContent = copied ? "程式碼已複製。" : "程式碼已選取，請使用系統的複製指令。";
        clearTimeout(resetStatus);
        resetStatus = setTimeout(() => {
            delete button.dataset.state;
            button.title = "複製程式碼";
        }, 1800);
    });
}
