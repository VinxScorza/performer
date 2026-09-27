(() => {
  const storageKey = "performer-docs-theme";
  const invertTheme = "invert";
  const stickyOffset = 10;
  const vinxFirmwareVersion = "v0.4.5";
  const vinxBetaVersion = "v0.4.6-beta.1";
  const vinxBetaDisplayVersion = "0.4.6";
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

  function syncVersionText() {
    document.querySelectorAll("[data-vinx-version]").forEach((node) => {
      node.textContent = vinxFirmwareVersion;
    });
    document.querySelectorAll("[data-vinx-beta-version]").forEach((node) => {
      node.textContent = vinxBetaVersion;
    });
    document.querySelectorAll("[data-vinx-beta-display-version]").forEach((node) => {
      node.textContent = vinxBetaDisplayVersion;
    });
  }

  function injectVersionBanner() {
    const mainContent = document.getElementById("main_content");
    if (!mainContent || mainContent.querySelector(":scope > .site-version-banner")) {
      return;
    }

    const banner = document.createElement("section");
    banner.className = "site-version-banner";
    banner.setAttribute("aria-label", "Firmware releases");

    const line = document.createElement("p");
    line.className = "site-version-banner-text";
    line.innerHTML = "Stable firmware: <strong><code data-vinx-version></code></strong> \u2022 Beta: <strong><code data-vinx-beta-version></code></strong> (module display: <code data-vinx-beta-display-version></code>) \u2022 <a href=\"https://github.com/VinxScorza/performer/releases\" target=\"_blank\" rel=\"noopener noreferrer\">Releases</a>";

    const warning = document.createElement("p");
    warning.className = "site-version-banner-warning";
    warning.innerHTML = "<strong>Beta project warning:</strong> saving a project in 0.4.6 upgrades it to Version41. Firmware 0.4.5 and earlier cannot reopen that saved file. Back up the SD card or keep a separate project copy before saving with the beta.";

    banner.appendChild(line);
    banner.appendChild(warning);
    mainContent.insertBefore(banner, mainContent.firstChild);
  }

  function slugifyFeatureTitle(text) {
    const slug = text
      .toLowerCase()
      .replace(/&/g, " and ")
      .replace(/[^a-z0-9]+/g, "-")
      .replace(/^-+|-+$/g, "");
    return slug || "section";
  }

  function openFeatureSectionFromHash() {
    const hash = window.location.hash;
    if (!hash || hash.length < 2) {
      return;
    }

    const id = decodeURIComponent(hash.slice(1));
    const target = document.getElementById(id);
    if (!target) {
      return;
    }

    const section = target.classList.contains("feature-block") ? target : target.closest(".feature-block");
    if (!section) {
      return;
    }

    const details = section.querySelector(":scope > details.feature-collapsible");
    if (details) {
      details.open = true;
    }
  }

  function enhanceFeaturesPage() {
    if (!document.body.classList.contains("features-page")) {
      return;
    }

    const mainContent = document.getElementById("main_content");
    if (!mainContent || mainContent.querySelector(".feature-collapsible")) {
      return;
    }

    const sections = Array.from(mainContent.querySelectorAll(":scope > section"));
    const featureSections = sections.filter((section) => section.querySelector(":scope > h4"));
    if (!featureSections.length) {
      return;
    }

    const usedIds = new Set(Array.from(document.querySelectorAll("[id]")).map((node) => node.id));

    const nav = document.createElement("nav");
    nav.className = "feature-anchor-nav";
    nav.setAttribute("aria-label", "Feature sections");

    const navTitle = document.createElement("p");
    navTitle.className = "feature-anchor-nav-title";
    navTitle.textContent = "Jump to section";

    const navList = document.createElement("ul");
    navList.className = "feature-anchor-nav-list";

    nav.appendChild(navTitle);
    nav.appendChild(navList);

    featureSections.forEach((section, index) => {
      const heading = section.querySelector(":scope > h4");
      if (!heading) {
        return;
      }

      const title = heading.textContent.trim();
      const baseId = "feature-" + slugifyFeatureTitle(title);
      let id = baseId;
      let suffix = 2;
      while (usedIds.has(id)) {
        id = baseId + "-" + suffix;
        suffix += 1;
      }
      usedIds.add(id);

      section.id = id;
      section.classList.add("feature-block");

      const details = document.createElement("details");
      details.className = "feature-collapsible";

      const summary = document.createElement("summary");
      summary.className = "feature-collapsible-summary";

      const titleNode = document.createElement("span");
      titleNode.className = "feature-collapsible-title";
      titleNode.textContent = title;

      const anchor = document.createElement("a");
      anchor.className = "feature-anchor-link";
      anchor.href = "#" + id;
      anchor.textContent = "#";
      anchor.setAttribute("aria-label", "Direct link to " + title);
      anchor.addEventListener("click", (event) => {
        event.preventDefault();
        event.stopPropagation();
        details.open = true;
        if (window.location.hash !== "#" + id) {
          window.location.hash = id;
        } else {
          openFeatureSectionFromHash();
        }
      });

      summary.appendChild(titleNode);
      summary.appendChild(anchor);

      const body = document.createElement("div");
      body.className = "feature-collapsible-body";

      const contentNodes = Array.from(section.children).filter((child) => child !== heading);
      contentNodes.forEach((node) => {
        body.appendChild(node);
      });

      heading.remove();
      details.appendChild(summary);
      details.appendChild(body);
      section.appendChild(details);

      const navItem = document.createElement("li");
      const navLink = document.createElement("a");
      navLink.href = "#" + id;
      navLink.textContent = title;
      navItem.appendChild(navLink);
      navList.appendChild(navItem);
    });

    mainContent.insertBefore(nav, featureSections[0]);
    openFeatureSectionFromHash();
    window.addEventListener("hashchange", openFeatureSectionFromHash);
  }

  document.addEventListener("DOMContentLoaded", () => {
    applyTheme(window.localStorage.getItem(storageKey) || "");
    injectVersionBanner();
    syncVersionText();
    enhanceFeaturesPage();
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
