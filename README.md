Après avoir installé `yacc`, `bison` et `flex` sur votre machine :

```bash
make # génère bin/structit
make backend # génère bin/structit_backend
make test
make test-validate # valide avec le backend
```

Pour l'instant, le compilateur génère autant de variables temporaires que nécessaire sans optimisation. La prochaine étape sera d'utiliser l'algorithme de Sethi-Ullman pour minimiser le nombre de registres en traversant l'AST.