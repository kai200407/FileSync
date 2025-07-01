let srcPath = '/';
let dstPath = '/';

async function listDir(path, treeId, onSelect) {
  const res = await fetch(`/list_dir?path=${encodeURIComponent(path)}`);
  const items = await res.json();
  const tree = document.getElementById(treeId);
  tree.innerHTML = '';
  items.forEach(item => {
    const el = document.createElement('div');
    el.className = item.is_dir ? 'dir' : 'file';
    el.textContent = item.name;
    if (item.is_dir) {
      el.onclick = () => {
        onSelect(path + (path.endsWith('/') ? '' : '/') + item.name);
      };
    }
    tree.appendChild(el);
  });
}

function selectSrcDir(path) {
  srcPath = path;
  fetch('/set_src_dir', {method: 'POST', body: JSON.stringify({path})});
  listDir(path, 'src-dir-tree', selectSrcDir);
}
function selectDstDir(path) {
  dstPath = path;
  fetch('/set_dst_dir', {method: 'POST', body: JSON.stringify({path})});
  listDir(path, 'dst-dir-tree', selectDstDir);
}

document.addEventListener('DOMContentLoaded', () => {
  listDir('/', 'src-dir-tree', selectSrcDir);
  listDir('/', 'dst-dir-tree', selectDstDir);
  document.getElementById('sync-btn').onclick = startSync;
  loadLogs();
  loadConflicts();
});

async function startSync() {
  document.getElementById('status').textContent = '同步中...';
  const res = await fetch('/sync', {method: 'POST'});
  const data = await res.json();
  document.getElementById('status').textContent = data.status || '同步完成';
  loadLogs();
  loadConflicts();
}

async function loadLogs() {
  const res = await fetch('/logs');
  document.getElementById('logs').textContent = await res.text();
}

async function loadConflicts() {
  const res = await fetch('/conflicts');
  const data = await res.json();
  const div = document.getElementById('conflicts');
  div.innerHTML = '';
  if (data.length === 0) {
    div.textContent = '无冲突';
    return;
  }
  data.forEach(conflict => {
    const el = document.createElement('div');
    el.textContent = `冲突文件: ${conflict.path}`;
    div.appendChild(el);
  });
} 