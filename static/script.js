async function setTarget() {
  const user = document.getElementById('user').value;
  const target = parseInt(document.getElementById('target').value);
  
  const res = await fetch('/set_target', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({user: user, target: target})
  });

  const j = await res.json();
  document.getElementById('latest').innerText = JSON.stringify(j);
}

async function getTarget() {
  const user = document.getElementById('user').value;
  const res = await fetch(`/get_target?user=${user}`);
  const j = await res.json();
  document.getElementById('latest').innerText = JSON.stringify(j);
}


async function refreshLogs() {
  const r = await fetch('/logs');
  const j = await r.json();
  if (j.length === 0) {
    document.getElementById('logs').innerText = 'No logs yet';
  } else {
    document.getElementById('logs').innerHTML =
      '<pre>' + JSON.stringify(j.slice(-20).reverse(), null, 2) + '</pre>';
  }
}


async function refreshNotifications() {
  const r = await fetch('/notifications');
  const j = await r.json();
  if (j.length === 0) {
    document.getElementById('notes').innerText = 'No notifications';
  } else {
    document.getElementById('notes').innerHTML =
      '<pre>' + JSON.stringify(j.slice(-20).reverse(), null, 2) + '</pre>';
  }
}


// refresh on load
refreshLogs();
refreshNotifications();