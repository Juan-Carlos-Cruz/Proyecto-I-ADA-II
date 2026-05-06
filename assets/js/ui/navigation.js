(function(global) {
  function goTo(screenName) {
    document.querySelectorAll(".screen").forEach(element => element.classList.remove("active"));
    document.getElementById("screen-" + screenName).classList.add("active");

    const stepMap = { welcome: 0, config: 1, result: 2 };

    document.querySelectorAll(".step-dot").forEach((dot, index) => {
      dot.classList.toggle("active", index === stepMap[screenName]);
    });

    document.querySelectorAll(".step-node").forEach((node, index) => {
      node.classList.toggle("active", index === stepMap[screenName]);
    });
  }

  function openFilePicker() {
    document.getElementById("fileInput").click();
  }

  function handleUploadKey(event) {
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      openFilePicker();
    }
  }

  global.RiegoNavigation = {
    goTo,
    openFilePicker,
    handleUploadKey
  };

  global.goTo = goTo;
  global.openFilePicker = openFilePicker;
  global.handleUploadKey = handleUploadKey;
})(window);
