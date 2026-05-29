(() => {
  "use strict";

  const html = document.documentElement;

  // ---------- language ----------
  const SUPPORTED = ["en", "zh"];
  const STORAGE_KEY = "apvlv.lang";

  function detectLang() {
    const saved = localStorage.getItem(STORAGE_KEY);
    if (saved && SUPPORTED.includes(saved)) return saved;
    const nav = (navigator.language || "en").toLowerCase();
    return nav.startsWith("zh") ? "zh" : "en";
  }

  function applyLang(lang) {
    html.lang = lang;
    document.querySelectorAll("[data-en][data-zh]").forEach((el) => {
      const text = el.getAttribute(`data-${lang}`);
      if (text == null) return;
      if (!el.querySelector("*")) {
        el.textContent = text;
      } else {
        let node = null;
        for (const n of el.childNodes) {
          if (n.nodeType === Node.TEXT_NODE && n.textContent.trim()) { node = n; break; }
        }
        if (node) node.textContent = text;
        else el.prepend(document.createTextNode(text));
      }
    });
    document.title =
      lang === "zh"
        ? "apvlv —— 用 Vim 的方式阅读文档"
        : "apvlv — Vim-like document reading";
  }

  let currentLang = detectLang();
  applyLang(currentLang);

  const langToggle = document.getElementById("lang-toggle");
  if (langToggle) {
    langToggle.addEventListener("click", () => {
      currentLang = currentLang === "en" ? "zh" : "en";
      localStorage.setItem(STORAGE_KEY, currentLang);
      applyLang(currentLang);
    });
  }

  // ---------- nav scroll state ----------
  const nav = document.getElementById("nav");
  const setScrolled = () => {
    if (!nav) return;
    nav.classList.toggle("is-scrolled", window.scrollY > 8);
  };
  setScrolled();
  window.addEventListener("scroll", setScrolled, { passive: true });

  // ---------- mobile menu ----------
  const menuToggle = document.getElementById("menu-toggle");
  if (menuToggle && nav) {
    menuToggle.addEventListener("click", () => {
      const open = nav.classList.toggle("menu-open");
      menuToggle.setAttribute("aria-expanded", String(open));
    });
    nav.querySelectorAll(".nav-links a").forEach((a) =>
      a.addEventListener("click", () => {
        nav.classList.remove("menu-open");
        menuToggle.setAttribute("aria-expanded", "false");
      })
    );
  }

  // ---------- tabs ----------
  document.querySelectorAll("[data-tabgroup]").forEach((group) => {
    const tabs = group.querySelectorAll(".tab");
    const panels = group.querySelectorAll(".tabpanel");
    tabs.forEach((tab) => {
      tab.addEventListener("click", () => {
        const id = tab.dataset.tab;
        tabs.forEach((t) => {
          const active = t === tab;
          t.classList.toggle("is-active", active);
          t.setAttribute("aria-selected", String(active));
        });
        panels.forEach((p) => p.classList.toggle("is-active", p.dataset.panel === id));
      });
    });
  });

  // ---------- copy buttons ----------
  document.querySelectorAll(".copy").forEach((btn) => {
    btn.addEventListener("click", async () => {
      const code = btn.closest(".code")?.querySelector("pre code");
      if (!code) return;
      const text = code.innerText;
      try {
        await navigator.clipboard.writeText(text);
        const original = btn.textContent;
        btn.textContent = currentLang === "zh" ? "已复制" : "Copied";
        btn.classList.add("is-copied");
        setTimeout(() => {
          btn.textContent = original;
          btn.classList.remove("is-copied");
        }, 1400);
      } catch (e) {
        btn.textContent = "!";
        setTimeout(() => (btn.textContent = currentLang === "zh" ? "复制" : "Copy"), 1400);
      }
    });
  });

  // ---------- keep copy button label in sync with language ----------
  const observer = new MutationObserver(() => {
    document.querySelectorAll(".copy:not(.is-copied)").forEach((b) => {
      b.textContent = currentLang === "zh" ? "复制" : "Copy";
    });
  });
  observer.observe(html, { attributes: true, attributeFilter: ["lang"] });
  document.querySelectorAll(".copy").forEach((b) => {
    b.textContent = currentLang === "zh" ? "复制" : "Copy";
  });

  // ---------- intersection-based fade-in ----------
  if ("IntersectionObserver" in window) {
    const io = new IntersectionObserver(
      (entries) => {
        entries.forEach((e) => {
          if (e.isIntersecting) {
            e.target.style.opacity = "1";
            e.target.style.transform = "translateY(0)";
            io.unobserve(e.target);
          }
        });
      },
      { rootMargin: "0px 0px -60px 0px", threshold: 0.05 }
    );
    document.querySelectorAll(".card, .backend, .setting, .qs-step, .cmd-panel").forEach((el) => {
      el.style.opacity = "0";
      el.style.transform = "translateY(14px)";
      el.style.transition = "opacity 0.6s cubic-bezier(0.4,0,0.2,1), transform 0.6s cubic-bezier(0.4,0,0.2,1)";
      io.observe(el);
    });
  }
})();
