// Falkon-OS Web OS & Documentation Interactive Engine

// Section Switching
function showSection(sectionId) {
    const sections = document.querySelectorAll('.doc-section');
    sections.forEach(sec => sec.classList.remove('active'));

    const activeSection = document.getElementById(sectionId);
    if (activeSection) {
        activeSection.classList.add('active');
    }

    const links = document.querySelectorAll('.sidebar-nav .nav-link');
    links.forEach(link => {
        link.classList.remove('active');
        if (link.getAttribute('href') === `#${sectionId}`) {
            link.classList.add('active');
        }
    });

    window.scrollTo({ top: 0, behavior: 'smooth' });
}

// Tab Switching
function switchTab(event, tabId) {
    const tabContainer = event.currentTarget.closest('.tab-container');
    const tabBtns = tabContainer.querySelectorAll('.tab-btn');
    const tabContents = tabContainer.querySelectorAll('.tab-content');

    tabBtns.forEach(btn => btn.classList.remove('active'));
    tabContents.forEach(content => content.classList.remove('active'));

    event.currentTarget.classList.add('active');
    const targetContent = tabContainer.querySelector(`#${tabId}`);
    if (targetContent) {
        targetContent.classList.add('active');
    }
}

// Theme Toggle
function toggleTheme() {
    const body = document.body;
    const btn = document.getElementById('themeToggle');

    if (body.classList.contains('dark-theme')) {
        body.classList.remove('dark-theme');
        body.classList.add('light-theme');
        btn.innerHTML = '🌙 Dark Mode';
    } else {
        body.classList.remove('light-theme');
        body.classList.add('dark-theme');
        btn.innerHTML = '☀️ Light Mode';
    }
}

// Cyberpunk Notification Toast System
function showToast(message) {
    const existing = document.querySelector('.cyber-toast');
    if (existing) existing.remove();

    const toast = document.createElement('div');
    toast.className = 'cyber-toast';
    toast.innerHTML = `<span>⚡</span> <span>[SYSTEM]: ${message}</span>`;
    document.body.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transition = 'opacity 0.5s ease';
        setTimeout(() => toast.remove(), 500);
    }, 3000);
}

// Live Search Filtering
function filterDocs() {
    const input = document.getElementById('searchInput').value.toLowerCase();
    const sections = document.querySelectorAll('.doc-section');
    
    if (!input.trim()) {
        const currentHash = window.location.hash.substring(1) || 'overview';
        showSection(currentHash);
        return;
    }

    sections.forEach(sec => {
        const text = sec.innerText.toLowerCase();
        if (text.includes(input)) {
            sec.classList.add('active');
        } else {
            sec.classList.remove('active');
        }
    });
}

// Handle Hash Links on Load
window.addEventListener('DOMContentLoaded', () => {
    const hash = window.location.hash.substring(1);
    if (hash) {
        showSection(hash);
    }
});
