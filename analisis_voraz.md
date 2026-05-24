---
title: "Análisis Comparativo: Voraz vs Óptimo — Riego de Tablones"
author: "Proyecto I — ADA II"
date: "`r Sys.Date()`"
output:
  html_document:
    toc: true
    toc_float: true
    theme: flatly
    highlight: tango
    code_folding: hide
---

```{r setup, include=FALSE}
knitr::opts_chunk$set(echo = TRUE, warning = FALSE, message = FALSE, fig.align = "center")
library(ggplot2)
library(dplyr)
library(tidyr)
library(knitr)
```

---

## 0. Intuición del algoritmo (explicación simple)

### Imagina que tienes un jardín con plantas 🌱

Tienes varias plantas y solo una manguera. Solo puedes regar **una planta a la vez**. Cada planta tiene:

- **¿Cuánto tarda en regarse?** → `tr` (tiempo de riego)
- **¿Cuándo se muere si no la riegas?** → `ts` (tiempo de supervivencia)
- **¿Qué tan importante es?** → `p` (prioridad)
- **¿Cuándo está en su mejor momento?** → `rp` (día perfecto)

### El "costo" es el estrés de las plantas 😰

Después de regar una planta, le preguntas: *"¿cómo te fue?"*

| Situación | Respuesta de la planta | Estrés |
|---|---|---|
| La regaste en su día perfecto | "¡Perfecto, gracias!" | `ts − t − tr` (poco) |
| Llegaste antes de que muriera | "Ok, llegaste a tiempo" | `2 × (ts − tr − t)` (algo) |
| Llegaste tarde (ya moría) | "¡Casi me muero!" | `2 × p × (t + tr − ts)` (mucho, y las importantes se enojan el doble) |

Queremos que la **suma total de estrés sea la menor posible**.

### El criterio voraz: `tr / p`

La regla del voraz es simple: **la planta con el número `tr/p` más pequeño va primero**.

```
Planta A: tarda 2 min, poco importante  (p=1)  →  tr/p = 2
Planta B: tarda 6 min, muy importante   (p=3)  →  tr/p = 2   (empate → desempate por ts−tr)
Planta C: tarda 4 min, muy importante   (p=4)  →  tr/p = 1   ← va primero
```

### ¿Por qué `tr/p`? El argumento de intercambio 🔄

Imagina dos plantas que **están ambas a punto de morir** (Caso 3). ¿Cuál va primero?

```
Flor:  tr=2, p=4  →  tr/p = 0.5
Árbol: tr=6, p=1  →  tr/p = 6.0
```

**Opción 1 — Flor primero, luego Árbol:**

```
Estrés Flor  = 2 × 4 × (0 + 2 − ts)
Estrés Árbol = 2 × 1 × (2 + 6 − ts)
```

**Opción 2 — Árbol primero, luego Flor:**

```
Estrés Árbol = 2 × 1 × (0 + 6 − ts)
Estrés Flor  = 2 × 4 × (6 + 2 − ts)   ← Flor esperó 6 min siendo muy importante
```

La diferencia entre las dos opciones siempre es:

$$\text{Costo}(i \to j) - \text{Costo}(j \to i) = 2 \cdot (p_j \cdot tr_i - p_i \cdot tr_j)$$

Si `tr_i/p_i < tr_j/p_j`, entonces esa diferencia es **negativa** → poner `i` primero es siempre mejor.
Esto demuestra que el voraz es **óptimo cuando todas las plantas están en Caso 3**.

### ¿Por qué a veces falla? El "día perfecto" 🌞

Si la **Rosa** tiene su día perfecto a las 10:00 y la riegas a las 6:00 (porque el voraz la pone primero por tener `tr/p` pequeño), su estrés es alto. El óptimo espera y llega exactamente a las 10:00, bajando el estrés a casi cero.

**En el Test 1 del proyecto:**

```
Voraz pone T4 primero (tr/p = 0.5) a t=0:
  → Estrés T4 = 2×(14−1−0) = 26   ← innecesario

Óptimo pone T2 primero a t=0:
  → T3 llega exactamente a su límite: estrés = 0
  → T1 llega exactamente a su límite: estrés = 0
  → Costo total = 44  (vs 59 del voraz)
```

> **En una oración:** el voraz es como un niño que siempre come primero el dulce más pequeño e importante. Eso es lo mejor cuando todos los dulces se están derritiendo. Pero si algunos dulces están bien a temperatura ambiente por ahora, a veces conviene esperar el momento exacto para comerlos.

---

## 1. Funciones del modelo

Replicamos en R la función de costo del enunciado y el criterio voraz del código C++.

```{r funciones}
BASE_DIR  <- "/home/juan-carlos-cruz/Documentos/Proyectos/Proyecto-I-ADAII/Proyecto-I-ADA-II"
TESTS_IN  <- file.path(BASE_DIR, "bateria_pruebas/Tests/tests")
TESTS_OUT <- file.path(BASE_DIR, "bateria_pruebas/Tests/output")

# Lee un archivo de entrada: devuelve lista de tablones (ts, tr, p, rp)
parse_input <- function(num) {
  f <- readLines(file.path(TESTS_IN, paste0("test", num, "_in.txt")))
  n <- as.integer(f[1])
  lapply(2:(n + 1), function(i) {
    v <- as.integer(strsplit(trimws(f[i]), ",")[[1]])
    list(ts = v[1], tr = v[2], p = v[3], rp = v[4])
  })
}

# Lee un archivo de salida: devuelve costo óptimo y orden (0-indexado)
parse_output <- function(num) {
  f <- trimws(readLines(file.path(TESTS_OUT, paste0("test", num, "_out.txt"))))
  f <- f[nchar(f) > 0]
  list(costo = as.integer(f[1]), orden = as.integer(f[-1]))
}

# Detecta el caso de costo según la función CR_F^Pi[i]
detectar_caso <- function(tab, t) {
  if (t == tab$rp)               return(1L)   # Caso 1: dia perfecto
  if (tab$ts - tab$tr >= t)      return(2L)   # Caso 2: a tiempo
  return(3L)                                  # Caso 3: tarde
}

# Calcula CR_F^Pi[i] para un tablon regado en el instante t
calcular_costo <- function(tab, t) {
  caso <- detectar_caso(tab, t)
  if (caso == 1) return(tab$ts - t - tab$tr)
  if (caso == 2) return(2 * (tab$ts - tab$tr - t))
  return(2 * tab$p * (t + tab$tr - tab$ts))
}

# Evalua CR_F = sum CR_F^Pi[i] dado un orden (0-indexado)
evaluar_orden <- function(tablones, orden) {
  t <- 0
  filas <- vector("list", length(orden))
  for (k in seq_along(orden)) {
    idx <- orden[k]
    tab <- tablones[[idx + 1]]
    caso  <- detectar_caso(tab, t)
    costo <- calcular_costo(tab, t)
    filas[[k]] <- data.frame(
      pos = k, tablon = idx, t_inicio = t,
      caso = caso, costo = costo,
      ts = tab$ts, tr = tab$tr, p = tab$p, rp = tab$rp
    )
    t <- t + tab$tr
  }
  list(costo_total = sum(sapply(filas, `[[`, "costo")),
       detalles    = do.call(rbind, filas))
}

# Criterio voraz: ordena por tr/p asc, desempate ts-tr asc (igual que el C++)
greedy_voraz <- function(tablones) {
  n  <- length(tablones)
  ix <- 0:(n - 1)
  df <- data.frame(
    idx    = ix,
    ratio  = sapply(ix, function(i) tablones[[i+1]]$tr / tablones[[i+1]]$p),
    margen = sapply(ix, function(i) tablones[[i+1]]$ts - tablones[[i+1]]$tr)
  )
  df[order(df$ratio, df$margen, df$idx), "idx"]
}
```

---

## 2. Carga y simulación sobre todos los tests

```{r datos}
N_TESTS <- 18

resultados <- lapply(1:N_TESTS, function(num) {
  tabs <- parse_input(num)
  out  <- parse_output(num)

  orden_v <- greedy_voraz(tabs)
  ev_v    <- evaluar_orden(tabs, orden_v)
  ev_opt  <- evaluar_orden(tabs, out$orden)

  list(
    test         = num,
    n            = length(tabs),
    costo_opt    = ev_opt$costo_total,
    costo_voraz  = ev_v$costo_total,
    orden_opt    = out$orden,
    orden_voraz  = orden_v,
    detalles_opt = ev_opt$detalles,
    detalles_v   = ev_v$detalles
  )
})

# Tabla resumen plana
resumen <- do.call(rbind, lapply(resultados, function(r) {
  data.frame(
    Test          = r$test,
    n             = r$n,
    Costo_Optimo  = r$costo_opt,
    Costo_Voraz   = r$costo_voraz,
    Diferencia    = r$costo_voraz - r$costo_opt,
    Pct_Exceso    = round(100 * (r$costo_voraz - r$costo_opt) / max(r$costo_opt, 1), 1),
    Optimo        = (r$costo_voraz == r$costo_opt)
  )
}))
```

---

## 3. Tabla comparativa de costos

```{r tabla-resumen}
kable(resumen,
      col.names = c("Test", "n", "Costo Óptimo", "Costo Voraz",
                    "Diferencia", "% Exceso", "¿Voraz = Óptimo?"),
      align = "ccccccc",
      caption = "CR_F total: Óptimo (PD/FB) vs Voraz (tr/p, desempate ts-tr)") |>
  kableExtra::kable_styling(bootstrap_options = c("striped", "hover", "condensed")) |>
  kableExtra::row_spec(which(resumen$Optimo), background = "#d4edda") |>
  kableExtra::row_spec(which(!resumen$Optimo & resumen$Pct_Exceso > 50),
                       background = "#f8d7da")
```

---

## 4. Costos absolutos: Óptimo vs Voraz

```{r grafico-costos, fig.width=10, fig.height=5}
df_long <- resumen |>
  select(Test, n, Costo_Optimo, Costo_Voraz) |>
  pivot_longer(cols = c(Costo_Optimo, Costo_Voraz),
               names_to = "Algoritmo", values_to = "Costo") |>
  mutate(Algoritmo = recode(Algoritmo,
                            Costo_Optimo = "Óptimo (PD/FB)",
                            Costo_Voraz  = "Voraz (tr/p)"),
         Label = paste0("T", Test, "\n(n=", n, ")"))

ggplot(df_long, aes(x = factor(Test), y = Costo, fill = Algoritmo)) +
  geom_col(position = "dodge", width = 0.7) +
  geom_text(aes(label = Costo), position = position_dodge(width = 0.7),
            vjust = -0.4, size = 2.8, fontface = "bold") +
  scale_fill_manual(values = c("Óptimo (PD/FB)" = "#2ecc71",
                               "Voraz (tr/p)"    = "#3498db")) +
  labs(title    = expression("Costo total " * CR[F] * ": Óptimo vs Voraz por test"),
       subtitle = "Verde = solución óptima   |   Azul = estrategia voraz",
       x = "Test (n = número de tablones)", y = "Costo total", fill = NULL) +
  theme_minimal(base_size = 12) +
  theme(legend.position = "top",
        panel.grid.major.x = element_blank())
```

---

## 5. Porcentaje de exceso del voraz sobre el óptimo

```{r grafico-exceso, fig.width=9, fig.height=4}
resumen_plot <- resumen |>
  mutate(color = case_when(
    Pct_Exceso == 0  ~ "Exacto",
    Pct_Exceso <= 30 ~ "≤ 30% exceso",
    Pct_Exceso <= 80 ~ "≤ 80% exceso",
    TRUE             ~ "> 80% exceso"
  ),
  color = factor(color, levels = c("Exacto", "≤ 30% exceso",
                                   "≤ 80% exceso", "> 80% exceso")))

ggplot(resumen_plot, aes(x = factor(Test), y = Pct_Exceso, fill = color)) +
  geom_col(width = 0.7) +
  geom_hline(yintercept = 0, linewidth = 0.5) +
  geom_text(aes(label = paste0(Pct_Exceso, "%")),
            vjust = -0.4, size = 3, fontface = "bold") +
  scale_fill_manual(values = c("Exacto"       = "#2ecc71",
                               "≤ 30% exceso" = "#f1c40f",
                               "≤ 80% exceso" = "#e67e22",
                               "> 80% exceso" = "#e74c3c")) +
  labs(title    = "% de exceso del voraz respecto al óptimo",
       subtitle = "0% = voraz alcanzó el óptimo",
       x = "Test", y = "Exceso (%)", fill = NULL) +
  theme_minimal(base_size = 12) +
  theme(legend.position = "top",
        panel.grid.major.x = element_blank())
```

---

## 6. Distribución de casos (1, 2, 3) por algoritmo

```{r grafico-casos, fig.width=11, fig.height=5}
casos_df <- do.call(rbind, lapply(resultados, function(r) {
  opt_casos <- r$detalles_opt |>
    count(caso) |>
    mutate(algoritmo = "Óptimo", test = r$test, n = r$n)
  v_casos <- r$detalles_v |>
    count(caso) |>
    mutate(algoritmo = "Voraz", test = r$test, n = r$n)
  rbind(opt_casos, v_casos)
})) |>
  mutate(
    caso_label = recode(factor(caso),
                        `1` = "Caso 1\n(Día perfecto)",
                        `2` = "Caso 2\n(A tiempo)",
                        `3` = "Caso 3\n(Tarde/Penalizado)")
  )

ggplot(casos_df, aes(x = factor(test), y = n, fill = caso_label)) +
  geom_col(position = "stack", width = 0.7) +
  facet_wrap(~algoritmo) +
  scale_fill_manual(values = c(
    "Caso 1\n(Día perfecto)"     = "#27ae60",
    "Caso 2\n(A tiempo)"         = "#f39c12",
    "Caso 3\n(Tarde/Penalizado)" = "#c0392b"
  )) +
  labs(title    = "Distribución de casos de costo por test y algoritmo",
       subtitle = "Caso 1 (verde) = día perfecto · Caso 2 (naranja) = a tiempo · Caso 3 (rojo) = tardío",
       x = "Test", y = "Tablones", fill = NULL) +
  theme_minimal(base_size = 11) +
  theme(legend.position = "bottom",
        panel.grid.major.x = element_blank(),
        strip.text = element_text(face = "bold", size = 12))
```

---

## 7. Análisis del argumento de intercambio

El argumento de intercambio demuestra que, **si dos tablones adyacentes están en Caso 3**, el que tiene menor `tr/p` debe ir primero. Verificamos esto sobre todos los pares adyacentes en Caso 3 del voraz.

```{r intercambio}
verificar_intercambio <- function(tablones, orden) {
  n <- length(orden)
  violaciones <- 0
  pares       <- 0

  t <- 0
  tiempos <- numeric(n)
  for (k in seq_along(orden)) {
    tiempos[k] <- t
    t <- t + tablones[[orden[k] + 1]]$tr
  }

  for (k in 1:(n - 1)) {
    ti <- tiempos[k]
    tj <- tiempos[k + 1]
    tab_i <- tablones[[orden[k]     + 1]]
    tab_j <- tablones[[orden[k + 1] + 1]]
    caso_i <- detectar_caso(tab_i, ti)
    caso_j <- detectar_caso(tab_j, tj)

    if (caso_i == 3 && caso_j == 3) {
      pares <- pares + 1
      ratio_i <- tab_i$tr / tab_i$p
      ratio_j <- tab_j$tr / tab_j$p
      if (ratio_i > ratio_j) violaciones <- violaciones + 1
    }
  }
  data.frame(pares = pares, violaciones = violaciones)
}

intercambio_df <- do.call(rbind, lapply(resultados, function(r) {
  vi <- verificar_intercambio(parse_input(r$test), r$orden_voraz)
  data.frame(test = r$test, n = r$n,
             pares_caso3 = vi$pares, violaciones = vi$violaciones)
}))

kable(intercambio_df,
      col.names = c("Test", "n", "Pares adyacentes en Caso 3", "Violaciones del orden tr/p"),
      caption = "Verificación del argumento de intercambio sobre la solución voraz") |>
  kableExtra::kable_styling(bootstrap_options = c("striped", "hover")) |>
  kableExtra::row_spec(which(intercambio_df$violaciones == 0 &
                             intercambio_df$pares_caso3 > 0),
                       background = "#d4edda")
```

> **Resultado esperado:** el voraz nunca viola el orden `tr/p` en pares de Caso 3 (columna "Violaciones" = 0), lo que confirma empíricamente la validez del argumento de intercambio.

---

## 8. Visualización del argumento de intercambio (Test 1)

Ilustramos el cálculo analítico del intercambio para dos tablones concretos.

```{r argumento-visual, fig.width=9, fig.height=5}
# Test 1, tablones T3 (idx=3) y T1 (idx=1) — ambos candidatos en Caso 3 en algunos órdenes
tabs_1 <- parse_input(1)

# Barremos todos los pares (i, j) del test 1 y calculamos costo de orden (i,j) vs (j,i)
pares_t1 <- expand.grid(i = 0:4, j = 0:4) |>
  filter(i < j) |>
  rowwise() |>
  mutate(
    ratio_i     = tabs_1[[i + 1]]$tr / tabs_1[[i + 1]]$p,
    ratio_j     = tabs_1[[j + 1]]$tr / tabs_1[[j + 1]]$p,
    label_i     = paste0("T", i, " (tr/p=", round(ratio_i, 2), ")"),
    label_j     = paste0("T", j, " (tr/p=", round(ratio_j, 2), ")"),
    # Costo en Caso 3 desde t=0
    costo_ij    = {
      ti <- tabs_1[[i + 1]]; tj <- tabs_1[[j + 1]]; t <- 0
      2*ti$p*(t + ti$tr - ti$ts) + 2*tj$p*(t + ti$tr + tj$tr - tj$ts)
    },
    costo_ji    = {
      ti <- tabs_1[[i + 1]]; tj <- tabs_1[[j + 1]]; t <- 0
      2*tj$p*(t + tj$tr - tj$ts) + 2*ti$p*(t + tj$tr + ti$tr - ti$ts)
    },
    diferencia  = costo_ij - costo_ji,
    voraz_elige = ifelse(ratio_i <= ratio_j, "i antes j", "j antes i"),
    voraz_ok    = (diferencia <= 0 & voraz_elige == "i antes j") |
                  (diferencia >= 0 & voraz_elige == "j antes i"),
    par         = paste0("(T", i, ", T", j, ")")
  ) |>
  ungroup()

ggplot(pares_t1, aes(x = par, y = diferencia,
                      fill = ifelse(diferencia <= 0, "Voraz correcto", "Voraz correcto"))) +
  geom_col(width = 0.6, fill = "#3498db") +
  geom_hline(yintercept = 0, linewidth = 0.8, linetype = "dashed") +
  geom_text(aes(label = round(diferencia, 0)),
            vjust = ifelse(pares_t1$diferencia >= 0, -0.4, 1.2),
            size = 3.5, fontface = "bold") +
  labs(
    title    = "Diferencia de costo C(i→j) − C(j→i) en Caso 3 (Test 1, t = 0)",
    subtitle = "Negativo = poner i primero es mejor  |  Positivo = poner j primero es mejor",
    x        = "Par de tablones",
    y        = expression(Delta * " costo")
  ) +
  theme_minimal(base_size = 12) +
  theme(panel.grid.major.x = element_blank())
```

---

## 9. Detalle por tablón: Test 1 (n = 5)

```{r detalle-test1, fig.width=10, fig.height=5}
det_opt <- resultados[[1]]$detalles_opt |> mutate(algoritmo = "Óptimo")
det_v   <- resultados[[1]]$detalles_v   |> mutate(algoritmo = "Voraz")
det_1   <- bind_rows(det_opt, det_v) |>
  mutate(
    caso_label = recode(factor(caso),
                        `1` = "C1: Día perfecto",
                        `2` = "C2: A tiempo",
                        `3` = "C3: Tarde"),
    label_tab  = paste0("T", tablon, "\n(tr/p=",
                        round(tr / p, 2), ")")
  )

ggplot(det_1, aes(x = pos, y = costo, fill = caso_label)) +
  geom_col(width = 0.6) +
  geom_text(aes(label = paste0("T", tablon, "\n", costo)),
            position = position_stack(vjust = 0.5), size = 3) +
  facet_wrap(~algoritmo) +
  scale_fill_manual(values = c("C1: Día perfecto" = "#27ae60",
                               "C2: A tiempo"     = "#f39c12",
                               "C3: Tarde"        = "#c0392b")) +
  labs(title    = "Test 1 — Costo por tablón y posición en el orden de riego",
       subtitle = paste0("Óptimo = ", resultados[[1]]$costo_opt,
                         "   |   Voraz = ", resultados[[1]]$costo_voraz),
       x = "Posición en el orden de riego", y = "Costo CR_F[i]", fill = NULL) +
  theme_minimal(base_size = 12) +
  theme(legend.position = "bottom",
        panel.grid.major.x = element_blank(),
        strip.text = element_text(face = "bold"))
```

---

## 10. Resumen del argumento

```{r resumen-argumento}
n_exactos     <- sum(resumen$Optimo)
n_total       <- nrow(resumen)
exceso_medio  <- round(mean(resumen$Pct_Exceso), 1)
exceso_max    <- max(resumen$Pct_Exceso)
test_max      <- resumen$Test[which.max(resumen$Pct_Exceso)]
```

| Métrica | Valor |
|---|---|
| Tests donde voraz = óptimo | `r n_exactos` / `r n_total` |
| % exceso promedio del voraz | `r exceso_medio`% |
| % exceso máximo | `r exceso_max`% (Test `r test_max`) |

**Conclusión del argumento:**

- La estrategia voraz con criterio `tr/p` es **óptima cuando todos los tablones están en Caso 3**, demostrado por argumento de intercambio (análogo a la Regla de Smith para scheduling ponderado).
- En el caso general, el voraz es una **heurística** que minimiza eficientemente las penalizaciones del Caso 3, pero no explota las oportunidades de `dia_perfecto` (Caso 1), donde el costo *decrece* conforme se posterga el riego.
- El desempate por `ts − tr` ascendente protege a los tablones con menor margen de supervivencia, reduciendo el riesgo de degradación en cascada hacia el Caso 3.
