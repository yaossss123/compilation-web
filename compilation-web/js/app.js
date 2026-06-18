// App routing
function switchExp(name) {
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.querySelectorAll('.nav-item').forEach(b => b.classList.remove('active'));
    document.getElementById('page-' + name).classList.add('active');
    document.querySelector('[data-exp="' + name + '"]').classList.add('active');
}

function readFileAsText(file, callback) {
    const reader = new FileReader();
    reader.onload = function(e) { callback(e.target.result); };
    reader.readAsText(file);
}
