#!/bin/bash
# Script de vérification et relance du tunnel Cloudflare
# À exécuter sur le serveur (192.168.1.17)
#
# Usage: ./check-tunnel.sh [--restart|-r]
#   Sans argument : diagnostic seulement.
#   Avec --restart : si le tunnel est down (pas de processus ET HTTP != 200),
#     relance via systemctl si le service existe, sinon via nohup.
#   Pour cron (ex. toutes les 10 min) : /chemin/check-tunnel.sh --restart

RESTART_IF_DOWN=0
[[ "$1" = "--restart" || "$1" = "-r" ]] && RESTART_IF_DOWN=1

echo "=========================================="
echo "Vérification du tunnel Cloudflare"
echo "=========================================="
echo ""

# 1. Vérifier si cloudflared est en cours d'exécution
echo "[1/4] Vérification des processus cloudflared..."
if pgrep -x "cloudflared" > /dev/null; then
    echo "✓ cloudflared est en cours d'exécution"
    ps aux | grep cloudflared | grep -v grep
else
    echo "✗ cloudflared n'est PAS en cours d'exécution"
fi
echo ""

# 2. Vérifier le service systemd (si installé)
echo "[2/4] Vérification du service systemd..."
if systemctl is-active --quiet cloudflared 2>/dev/null; then
    echo "✓ Service cloudflared actif"
    systemctl status cloudflared --no-pager -l | head -10
else
    echo "✗ Service cloudflared inactif ou non installé"
fi
echo ""

# 3. Tester la connectivité Matrix
echo "[3/4] Test de connectivité vers matrix.buffertavern.com..."
response=$(curl -s -o /dev/null -w "%{http_code}" https://matrix.buffertavern.com/_matrix/client/versions 2>/dev/null)
if [ "$response" = "200" ]; then
    echo "✓ Serveur Matrix accessible (HTTP $response)"
else
    echo "✗ Serveur Matrix non accessible (HTTP $response)"
    echo "  Cela confirme que le tunnel n'est pas actif"
fi
echo ""

# 4. Options de relance
echo "[4/4] Options de relance :"
echo ""
echo "Pour relancer le tunnel manuellement :"
echo "  cloudflared tunnel run matrix"
echo ""
echo "Pour relancer via systemd (si installé) :"
echo "  sudo systemctl restart cloudflared"
echo "  sudo systemctl status cloudflared"
echo ""
echo "Pour vérifier les logs :"
echo "  journalctl -u cloudflared -f"
echo "  # ou si en mode manuel, regarder la sortie du processus"
echo ""

# 5. [Mode --restart] Relancer si down : pas de processus ET HTTP != 200
if [[ "$RESTART_IF_DOWN" -eq 1 ]]; then
    p=$(pgrep -x "cloudflared" 2>/dev/null)
    r=$(curl -s -o /dev/null -w "%{http_code}" https://matrix.buffertavern.com/_matrix/client/versions 2>/dev/null)
    if [[ -z "$p" && "$r" != "200" ]]; then
        echo "[--restart] Tunnel down (pgrep=vide, HTTP=$r). Relance..."
        if systemctl cat cloudflared &>/dev/null; then
            sudo systemctl restart cloudflared && echo "  -> systemctl restart cloudflared OK" || echo "  -> systemctl restart cloudflared ECHEC"
        else
            nohup cloudflared tunnel run matrix >>/tmp/cloudflared.log 2>&1 </dev/null & disown 2>/dev/null
            echo "  -> nohup cloudflared tunnel run matrix (log: /tmp/cloudflared.log)"
        fi
    else
        echo "[--restart] Tunnel up (pgrep=${p:-vide}, HTTP=$r), aucune action."
    fi
fi
