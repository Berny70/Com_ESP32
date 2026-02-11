/* ================================
   VERSION SERVICE WORKER
================================ */

const SW_VERSION = "1.2.0";
const CACHE_NAME = "compteurs-cache-" + SW_VERSION;

/* Fichiers à mettre en cache */
const STATIC_ASSETS = [
  "/",
  "/index.html",
  "/manifest.json"
];

/* ================================
   INSTALL
================================ */

self.addEventListener("install", event => {
  console.log("SW installé - version", SW_VERSION);

  self.skipWaiting();

  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(cache => {
        return cache.addAll(STATIC_ASSETS);
      })
  );
});

/* ================================
   ACTIVATE
================================ */

self.addEventListener("activate", event => {
  console.log("SW activé - nettoyage cache");

  event.waitUntil(
    caches.keys().then(keys => {
      return Promise.all(
        keys.map(key => {
          if (key !== CACHE_NAME) {
            console.log("Suppression ancien cache:", key);
            return caches.delete(key);
          }
        })
      );
    })
  );

  return self.clients.claim();
});

/* ================================
   FETCH
================================ */

self.addEventListener("fetch", event => {

  const url = new URL(event.request.url);

  /* Ne PAS cacher l'API */
  if (url.pathname.startsWith("/api")) {
    event.respondWith(fetch(event.request));
    return;
  }

  /* Cache First pour fichiers statiques */
  event.respondWith(
    caches.match(event.request)
      .then(response => {
        return response || fetch(event.request);
      })
      .catch(() => {
        /* Fallback offline simple */
        if (event.request.destination === "document") {
          return caches.match("/index.html");
        }
      })
  );
});
