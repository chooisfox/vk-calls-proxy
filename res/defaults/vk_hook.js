(() => {
  "use strict";
  if (window !== window.top || window.__vkHookInstalled) return;
  window.__vkHookInstalled = true;

  const WS_URL = "ws://127.0.0.1:%1";
  const log = (...args) => console.log("[VK-TUNNEL]", ...args);

  let activeWS = null,
    activeDC = null,
    dcOpen = false,
    isWsPaused = false;
  const HIGH_WM = 1024 * 1024,
    LOW_WM = 256 * 1024;

  const OrigPC = window.RTCPeerConnection;
  window.RTCPeerConnection = function (config) {
    const pc = new OrigPC(config);
    pc.addEventListener("connectionstatechange", () => {
      if (pc.connectionState === "connected") {
        log("[SYSTEM] WebRTC Connected. Initializing DataChannel...");
        setupDataChannel(pc);
      } else if (
        ["disconnected", "failed", "closed"].includes(pc.connectionState)
      ) {
        log("[SYSTEM] WebRTC Disconnected.");
        if (activeDC) {
          activeDC.close();
          activeDC = null;
        }
        dcOpen = false;
      }
    });
    return pc;
  };
  Object.assign(window.RTCPeerConnection, OrigPC);
  window.RTCPeerConnection.prototype = OrigPC.prototype;

  function setupDataChannel(pc) {
    if (pc.signalingState === "closed") return;
    try {
      const dc = pc.createDataChannel("tunnel", {
        negotiated: true,
        id: 2,
        ordered: true,
      });
      dc.binaryType = "arraybuffer";
      dc.bufferedAmountLowThreshold = LOW_WM;
      bindDataChannel(dc);
    } catch (e) {
      log("[ERROR] Failed to create DataChannel:", e);
    }
  }

  function sendGlobalControl(type) {
    if (!activeWS || activeWS.readyState !== WebSocket.OPEN) return;
    const buf = new ArrayBuffer(5);
    new DataView(buf).setUint8(4, type);
    activeWS.send(buf);
  }

  function bindDataChannel(dc) {
    if (activeDC && activeDC.readyState === "open") activeDC.close();
    activeDC = dc;
    dc.onopen = () => {
      dcOpen = true;
      connectWebSocket();
    };
    dc.onclose = () => {
      dcOpen = false;
    };
    dc.onbufferedamountlow = () => {
      if (isWsPaused) {
        isWsPaused = false;
        sendGlobalControl(7);
      }
    };
    dc.onmessage = (e) => {
      if (activeWS && activeWS.readyState === WebSocket.OPEN)
        activeWS.send(e.data);
    };
  }

  function connectWebSocket() {
    if (activeWS) {
      activeWS.onclose = null;
      activeWS.close();
    }
    activeWS = new WebSocket(WS_URL);
    activeWS.binaryType = "arraybuffer";
    activeWS.onclose = () => {
      log("[SYSTEM] Local WS connection lost. Reconnecting in 1s...");
      setTimeout(() => {
        if (dcOpen) connectWebSocket();
      }, 1000);
    };
    activeWS.onmessage = (e) => {
      if (activeDC && dcOpen) {
        activeDC.send(e.data);
        if (!isWsPaused && activeDC.bufferedAmount > HIGH_WM) {
          isWsPaused = true;
          sendGlobalControl(6);
        }
      }
    };
  }
})();
