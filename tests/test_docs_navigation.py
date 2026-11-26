import re
import unittest
from html import unescape
from pathlib import Path


def visible_text(markup):
    return " ".join(unescape(re.sub(r"<[^>]+>", "", markup)).split())


class DocsNavigationTest(unittest.TestCase):
    def test_sidebar_labels_match_their_section_titles(self):
        page = (Path(__file__).resolve().parents[1] / "docs/index.html").read_text(
            encoding="utf-8"
        )
        nav = re.search(r"<nav\b[^>]*>(.*?)</nav>", page, re.S)
        self.assertIsNotNone(nav)
        sidebar = re.findall(r'<a\b[^>]*\bhref="(#[^"]+)"[^>]*>(.*?)</a>', nav[1], re.S)
        self.assertTrue(sidebar)
        headings = {
            f"#{anchor}": visible_text(title)
            for anchor, title in re.findall(
                r'<h[1-6]\b[^>]*\sid="([^"]+)"[^>]*>(.*?)</h[1-6]>',
                page,
                re.S,
            )
        }
        for target, label in sidebar:
            with self.subTest(target=target):
                self.assertIn(target, headings)
                self.assertEqual(visible_text(label), headings[target])


if __name__ == "__main__":
    unittest.main()
