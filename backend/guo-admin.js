'use strict';
(() => {
  const nav = document.querySelector('#app nav');
  const tab = document.createElement('button');
  tab.dataset.tab = 'guo';
  tab.textContent = '奶蛙统计';
  nav.append(tab);
  const section = document.createElement('section');
  section.id = 'guo';
  section.hidden = true;
  const heading = document.createElement('h2');
  heading.textContent = '奶蛙使用统计';
  const note = document.createElement('p');
  note.textContent = '插件启动后自动上报；红果 ID 由客户端读取，不代表平台身份认证。人数按 ID 去重，未登录的安装只计入设备。日趋势保留 90 天，180 天未活跃的安装自动删除。离线和无法联网的安装尚未计入。';
  const metrics = document.createElement('div');
  metrics.className = 'metrics';
  const versions = document.createElement('div');
  const search = document.createElement('input');
  search.placeholder = '搜索红果 ID';
  search.setAttribute('aria-label', '搜索红果 ID');
  search.maxLength = 20;
  const summary = document.createElement('p');
  summary.setAttribute('role', 'status');
  const table = document.createElement('table');
  const head = table.createTHead().insertRow();
  for (const label of ['红果 ID', '红果版本', '奶蛙版本', 'iOS 版本', '首次使用', '最近使用']) {
    const th = document.createElement('th'); th.textContent = label; head.append(th);
  }
  const rows = table.createTBody();
  const scroll = document.createElement('div'); scroll.className = 'scroll'; scroll.append(table);
  const days = document.createElement('table');
  const dayhead = days.createTHead().insertRow();
  for (const label of ['日期（UTC+8）', '活跃人数', '活跃安装']) { const th = document.createElement('th'); th.textContent = label; dayhead.append(th); }
  const dayrows = days.createTBody();
  let offset = 0;
  const previous = button('上一页', async () => { offset = Math.max(0, offset - 100); await refresh(); });
  const next = button('下一页', async () => { offset += 100; await refresh(); });
  const refreshButton = button('查询 / 刷新', async () => { offset = 0; await refresh(); });
  search.onkeydown = event => { if (event.key === 'Enter') refreshButton.click(); };
  section.append(heading, note, metrics, versions, days, search, refreshButton, summary, scroll, previous, next);
  document.querySelector('#app').append(section);
  async function refresh() {
    const [stats, data] = await Promise.all([api('admin/guo/stats'), api('admin/guo/users?q=' + encodeURIComponent(search.value.trim()) + '&offset=' + offset)]);
    metrics.replaceChildren();
    for (const [key, title] of Object.entries({ users: '去重红果人数', installations: '安装数', activeToday: '今日活跃人数', active7d: '近 7 天活跃人数', onlineInstallations: '近 10 分钟上报安装' })) {
      const card = document.createElement('div'); card.className = 'metric';
      const label = document.createElement('span'); label.textContent = title;
      const value = document.createElement('strong'); value.textContent = stats[key]; card.append(label, value); metrics.append(card);
    }
    versions.replaceChildren();
    for (const [key, title] of Object.entries({ host_version: '红果版本', plugin_version: '奶蛙版本', system_version: 'iOS 版本' })) {
      const label = document.createElement('h3'); label.textContent = title + '（近 30 天活跃安装）'; versions.append(label);
      const values = document.createElement('p'); values.textContent = stats.versions[key].map(item => item.version + '：' + item.count).join(' · ') || '暂无数据'; versions.append(values);
    }
    dayrows.replaceChildren();
    for (const day of stats.days) { const row = dayrows.insertRow(); for (const value of [day.day, day.users, day.installations]) row.insertCell().textContent = value; }
    rows.replaceChildren();
    for (const user of data.items) {
      const row = rows.insertRow();
      for (const value of [user.hongguo_id || '未登录 / 未读取', user.host_version, user.plugin_version, user.system_version, new Date(user.first_seen * 1000).toLocaleString(), new Date(user.last_seen * 1000).toLocaleString()]) row.insertCell().textContent = value;
    }
    summary.textContent = `共 ${data.total} 个安装；${data.items.length ? offset + 1 : 0}–${offset + data.items.length}，更新于 ${new Date(stats.at * 1000).toLocaleString()}`;
    previous.disabled = offset === 0; next.disabled = offset + 100 >= data.total;
  }
  document.querySelectorAll('#app nav button').forEach(item => item.addEventListener('click', () => { section.hidden = item !== tab; }));
  tab.onclick = safely(async () => {
    document.querySelectorAll('#app > section').forEach(item => { item.hidden = item !== section; });
    await refresh();
  });
})();
