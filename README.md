# Ꮪⳑⲁⲩ 🙵 Ⲥⲟⲛჹⳙⲉⲅ

Slay & Conquer est un jeu de stratégie tour par tour, inspiré de Slay et Civilization.

### Pilotes de votre carte graphique
Avant tout, il n'est pas nécessaire d'avoir une carte graphique dédiée (nvidia, amd radeon...) pour lancer Slay & Conquer: le plupart des processeurs récent possèdent une carte graphique integré qui prend en charge vulkan.

Si vous possedez une carte nvidia, il faudra utiliser le pilote propriétaire, car le pilote `nouveau` ne prend pour l'instant pas en charge vulkan.
Référez vous au liens suivants pour installer les drivers nécessaires:

+ [Arch Linux](https://wiki.archlinux.org/index.php/xorg#Driver_installation) ([Vulkan](https://wiki.archlinux.org/index.php/Vulkan))
+ [Debian](https://wiki.debian.org/GraphicsCard) ([Vulkan](https://packages.debian.org/search?keywords=vulkan))
+ [Ubuntu](https://doc.ubuntu-fr.org/carte_graphique)

([Article décrivant l'installation de vulkan sur d'autre distributions](https://linuxconfig.org/install-and-test-vulkan-on-linux))


## Compilation du projet

Pour compiler, le projet nécessite deux dépendances: le SDK Vulkan, et Qt 5.12.1, avec la prise en charge Vulkan activé. Nous avons crée un script qui permet d'installer automatiquement une version portable de ces deux dépendances (donc installé dans le dossier du projet, pas sur votre système). Nous expliquerons la compilation uniquement via ce script. Si vous souhaitez compiler vous même le projet, referez vous à la documentation spécifique à votre distribution de linux. Le script ne fonctionnera que sur des architectures x86-64bits.

### Compilation et lancement du projet

Lancez un terminal à la racine du projet puis lancez:

```
chmod +x setup_compile_et_lance.sh
./setup_compile_et_lance.sh
```

Ce script va s'assurer que vous avez toutes les dépendances requises.

Il téléchargera ```qt5.zip```, une archive contenant la version spécifique de Qt nécessaire (Qt 5.12.2, avec la prise en charge Vulkan activé), le SDK Vulkan, ainsi que `libicu:63` et `libdouble-conversion:3`.

La compilation se lancera, et si il n'y a pas d'erreur le script vous proposera de lancer le jeu.

### Fichier de configuration

Le chemin du fichier de configuration est:`$HOME/.config/Slay3/SlayAndConquer.conf`

Il permet de customiser:

+ Les touches pour se déplacer
+ La résolution du jeu
+ La caméra du jeu

### En cas de problèmes

Vérifiez d'abord que vulkan fonctionne correctement en lançant ```vulkaninfo``` (Si vulkaninfo affiche un erreur, c'est qu'il y a un problème avec votre installation de vulkan).

Il est également possible que la version de ```qt5``` installé par le script de compilation ne soit pas à jour; Supprimez le dossier ```qt5``` situé à la racine du projet, faites ```make distclean``` pour nettoyer tout les fichiers compilés, et relancez ```setup_compile_et_lance.sh``` afin de re-télécharger et recompiler le projet.

## Mécanique du jeu:

### Condition de victoire :
Domination (c’est-à-dire élimination de toute les capitales)

### Carte :
La carte est généré de manière procédurale, et est de taille finie. Les villes sont également générés aléatoirement (mais de manière à ne pas donner d'avantage).

### Les joueurs :
Plusieurs joueurs tour-par-tour, et des I.A.

### Tour :
Début de tour : les différentes villes génèrent un certain nombre d'or lié en fonction de leur niveau.
Une partie de cet or est consommé par les unités présentes. Si il n'y a pas assez d'or, une ou plusieurs (en fonction de l'ampleur du deficit) unités au hasard désertent.


### Or :
La monnaie centrale du jeu. Sert à:

+ Acheter des unités
+ Payer les unités (à chaque tour)

### Unités :
Caractéristiques:

+ Point de vie (si l'unité ne bouge pas et n'attaque pas pendant un tour, elle regagne de la vie).
+ Point d’attaque
+ Point de défense
+ Portée d'attaque
+ Mobilité (portée de mouvement)
+ Hauteur de mouvement: la hauteur qu'une unité peut monter (par exemple les cavaliers ne peuvent pas grimper une montagne comme le ferai un soldat à pieds)

### Case :
Hexagonale; Peu contenir:

+ Une ville
+ Une unité

### Ville :
Donne une certaine quantité d'or (proportionel à la rentabilité de la ville) à chaque début tour au joueur qui la possède. Si un autre joueur place une unité sur une ville et y reste pendant le tour du joueur qui possède la ville, alors la ville est conquise; la ville devienne la propriété du joueur qui l'a prise.
