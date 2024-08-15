function loadMenu() {
    fetch('submenu.html')
        .then(response => response.text())
        .then(data => {
            document.getElementById('sidebar').innerHTML = data;
        })
        .catch(error => console.error('Error loading menu:', error));
}
