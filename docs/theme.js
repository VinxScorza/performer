(() => {
  const storageKey = "performer-docs-theme";
  const invertTheme = "invert";
  const stickyOffset = 10;
  let stickyButtons = [];

  function applyTheme(theme) {
    const root = document.documentElement;
    if (theme === invertTheme) {
      root.setAttribute("data-theme", invertTheme);
    } else {
      root.removeAttribute("data-theme");
    }
  }

  function currentTheme() {
    return document.documentElement.getAttribute("data-theme") || "";
  }

  function nextTheme() {
    return currentTheme() === invertTheme ? "" : invertTheme;
  }

  function buttonLabel(theme) {
    return theme === invertTheme ? "Default Colors" : "Invert Colors";
  }

  function updateButtons() {
    const theme = currentTheme();
    const pressed = theme === invertTheme ? "true" : "false";
    const label = buttonLabel(theme);

    document.querySelectorAll(".site-theme-toggle").forEach((button) => {
      button.setAttribute("aria-pressed", pressed);
      button.textContent = label;
    });
  }

  function persistTheme(theme) {
    if (theme) {
      window.localStorage.setItem(storageKey, theme);
    } else {
      window.localStorage.removeItem(storageKey);
    }
  }

  function toggleTheme() {
    const theme = nextTheme();
    applyTheme(theme);
    persistTheme(theme);
    updateButtons();
  }

  function recalculateStickyButtons() {
    stickyButtons.forEach((entry) => {
      entry.button.classList.remove("is-sticky");
      entry.triggerTop = entry.button.getBoundingClientRect().top + window.scrollY - stickyOffset;
    });

    updateStickyButtons();
  }

  function updateStickyButtons() {
    stickyButtons.forEach((entry) => {
      entry.button.classList.toggle("is-sticky", window.scrollY >= entry.triggerTop);
    });
  }

  document.addEventListener("DOMContentLoaded", () => {
    applyTheme(window.localStorage.getItem(storageKey) || "");
    updateButtons();

    stickyButtons = Array.from(document.querySelectorAll(".site-theme-toggle, .sticky-action-button, .site-nav")).map((button) => ({
      button,
      triggerTop: 0,
    }));

    document.querySelectorAll(".site-theme-toggle").forEach((button) => {
      button.addEventListener("click", toggleTheme);
    });

    recalculateStickyButtons();
    window.addEventListener("scroll", updateStickyButtons, { passive: true });
    window.addEventListener("resize", recalculateStickyButtons);
  });
})();
