(() => {
  "use strict";
  if (window.__vkAutoCallInstalled) return;
  window.__vkAutoCallInstalled = true;

  const log = (...args) => console.log("[VK-AUTO]", ...args);

  let activeSupervisor = null;
  let isMonitoringCall = false;

  function inviteUserToCall(targetUser) {
    if (!targetUser) return;
    log("Starting invite process for: " + targetUser);
    let attempts = 0;

    let inviteInterval = setInterval(() => {
      attempts++;
      if (attempts > 30) {
        clearInterval(inviteInterval);
        log("Failed to find invite modal after 15s");
        return;
      }

      let searchInput = document.querySelector(
        '[data-testid="calls_add_participants_search"]',
      );
      if (!searchInput) {
        let addBtn = document.querySelector(
          '[data-testid="call_management_toolbar_button_add_to_call"]',
        );
        if (!addBtn) {
          let fallbackBtn = document.querySelector(
            '[data-testid="calls_call_footer_button_participants"]',
          );
          if (fallbackBtn)
            addBtn = fallbackBtn.closest("button") || fallbackBtn;
        }
        if (addBtn) addBtn.click();
        return;
      }

      let isIdSearch = /^(id)?\d+$/i.test(targetUser.trim());
      let numericId = targetUser.replace(/\D/g, "");

      if (isIdSearch) {
        let targetRow = document.querySelector(
          `div[role="button"][data-id="${numericId}"]`,
        );
        if (targetRow) {
          clearInterval(inviteInterval);
          targetRow.click();
          clickConfirmAdd();
        } else {
          let showMoreBtns = Array.from(
            document.querySelectorAll('div[role="button"]'),
          ).filter(
            (el) =>
              el.innerText &&
              (el.innerText.includes("Показать еще") ||
                el.innerText.includes("Показать ещё")),
          );
          if (showMoreBtns.length > 0) showMoreBtns[0].click();
        }
      } else {
        const nativeInputValueSetter = Object.getOwnPropertyDescriptor(
          window.HTMLInputElement.prototype,
          "value",
        ).set;
        if (searchInput.value !== targetUser) {
          nativeInputValueSetter.call(searchInput, targetUser);
          searchInput.dispatchEvent(new Event("input", { bubbles: true }));
          searchInput.dispatchEvent(new Event("change", { bubbles: true }));
        }

        let queryLower = targetUser.toLowerCase().trim();
        let items = document.querySelectorAll(
          '[data-testid="calls_participant_list_item_name"]',
        );
        let targetItem = Array.from(items).find((el) =>
          el.innerText.trim().toLowerCase().includes(queryLower),
        );

        if (targetItem) {
          clearInterval(inviteInterval);
          let row = targetItem.closest('[role="button"]');
          if (row) row.click();
          clickConfirmAdd();
        }
      }
    }, 500);
  }

  function clickConfirmAdd() {
    let confirmInterval = setInterval(() => {
      let confirmBtns = Array.from(
        document.querySelectorAll(".CallsModalFooter button"),
      ).filter((b) => b.innerText.includes("Добавить") && !b.disabled);
      if (confirmBtns.length > 0) {
        confirmBtns[0].click();
        clearInterval(confirmInterval);
        log("User successfully invited!");
      }
    }, 500);
  }

  function extractLinkAndInvite(targetUser) {
    let attempts = 0;
    let findLinkInterval = setInterval(() => {
      attempts++;
      if (attempts > 20) {
        clearInterval(findLinkInterval);
        inviteUserToCall(targetUser);
        return;
      }

      let linkSettingsBtn = document.querySelector(
        '[data-testid="calls_call_link_settings_button"]',
      );
      if (linkSettingsBtn) {
        clearInterval(findLinkInterval);
        linkSettingsBtn.click();

        let getLinkInterval = setInterval(() => {
          let input = document.querySelector(
            '[data-testid="calls_link_settings_plain_link_input"]',
          );
          if (input && input.value && input.value.includes("vk.com/call")) {
            clearInterval(getLinkInterval);
            console.log("[VK-CALL-LINK] " + input.value);

            let closeBtn =
              document.querySelector(
                '[data-testid="calls_modal_header_close_button"]',
              ) || document.querySelector(".CallsModalHeader__cross");
            if (closeBtn) closeBtn.click();
            setTimeout(() => inviteUserToCall(targetUser), 500);
          }
        }, 500);
      }
    }, 500);
  }

  window.addEventListener("message", (event) => {
    if (typeof event.data !== "string") return;

    if (event.data.startsWith("START_CREATOR")) {
      const targetUser = event.data.split("|")[1] || "";
      log("Initializing Creator Supervisor loop...");

      if (activeSupervisor) clearInterval(activeSupervisor);
      isMonitoringCall = false;

      activeSupervisor = setInterval(() => {
        let isCallUIActive = document.querySelector(
          '[data-testid="calls_call_main_modal_root"]',
        );

        if (isCallUIActive) {
          if (!isMonitoringCall) {
            isMonitoringCall = true;
            log("Call is active. Ensuring target is invited...");
            extractLinkAndInvite(targetUser);
          }
          return;
        }

        if (isMonitoringCall && !isCallUIActive) {
          log("[RECONNECT] Call ended or was canceled. Re-hosting...");
          isMonitoringCall = false;
          if (window.location.pathname !== "/calls") {
            window.location.href = "https://vk.com/calls";
          }
          return;
        }

        let createBtn = document.querySelector(
          '[data-testid="calls_main_page_button_create_call"]',
        );
        if (createBtn) {
          createBtn.click();
          let modalInterval = setInterval(() => {
            let startBtn = document.querySelector(
              '[data-testid="calls_create_call_modal_start_call_button"]',
            );
            if (startBtn) {
              startBtn.click();
              clearInterval(modalInterval);
            }
          }, 500);
        } else if (!window.location.pathname.startsWith("/call")) {
          window.location.href = "https://vk.com/calls";
        }
      }, 2000);
    } else if (event.data === "STOP_CREATOR") {
      log("Halting Creator Supervisor loop.");
      if (activeSupervisor) clearInterval(activeSupervisor);
      activeSupervisor = null;
      isMonitoringCall = false;
    } else if (event.data === "START_CLIENT_USER") {
      log("Initializing Client Auto-Join Supervisor loop...");

      if (activeSupervisor) clearInterval(activeSupervisor);
      isMonitoringCall = false;

      activeSupervisor = setInterval(() => {
        let isCallUIActive = document.querySelector(
          '[data-testid="calls_call_main_modal_root"]',
        );

        if (isCallUIActive) {
          if (!isMonitoringCall) {
            log("Successfully joined the active call.");
            isMonitoringCall = true;
          }
          return;
        }

        if (isMonitoringCall && !isCallUIActive) {
          log(
            "[RECONNECT] Call canceled or dropped by host. Awaiting new call...",
          );
          isMonitoringCall = false;
          if (!window.location.href.includes("section=current")) {
            window.location.href = "https://vk.com/calls?section=current";
          }
          return;
        }

        let joinBtn = document.querySelector(
          '[data-testid="calls_current_calls_join"]',
        );
        if (joinBtn) {
          joinBtn.click();
          log("Incoming call detected. Accepting connection...");

          let confirmInterval = setInterval(() => {
            let confirmBtn = document.querySelector(
              '[data-testid="calls_preview_join_button"]',
            );
            if (confirmBtn) {
              confirmBtn.click();
              clearInterval(confirmInterval);
            }
          }, 500);
        } else if (
          !window.location.href.includes("section=current") &&
          !window.location.pathname.startsWith("/call")
        ) {
          window.location.href = "https://vk.com/calls?section=current";
        }
      }, 1500);
    } else if (event.data === "STOP_CLIENT_USER") {
      log("Halting Client Auto-Join Supervisor loop.");
      if (activeSupervisor) clearInterval(activeSupervisor);
      activeSupervisor = null;
      isMonitoringCall = false;
    } else if (event.data === "START_CLIENT_ANON") {
      let joinInterval = setInterval(() => {
        let input = document.querySelector(
          'input[placeholder="Your name"], input[placeholder="Ваше имя"]',
        );
        let joinBtn = document.querySelector(
          '[data-testid="calls_preview_join_button_anonym"]',
        );
        if (input && joinBtn) {
          clearInterval(joinInterval);
          const setter = Object.getOwnPropertyDescriptor(
            window.HTMLInputElement.prototype,
            "value",
          ).set;
          setter.call(input, "ProxyUser_" + Math.floor(Math.random() * 1000));
          input.dispatchEvent(new Event("input", { bubbles: true }));
          input.dispatchEvent(new Event("change", { bubbles: true }));
          setTimeout(() => joinBtn.click(), 500);
        }
      }, 500);
    }
  });
})();
