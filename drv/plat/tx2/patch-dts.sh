#!/bin/env bash

patch ./tegra186.dts << EOF
--- tegra186.dts
+++ tegra186.dts
@@ -583,13 +583,13 @@
 		spi0 = "/spi@3210000";
 		spi1 = "/spi@c260000";
 		spi2 = "/spi@3230000";
-		spi3 = "/spi@3240000";
+		// spi3 = "/spi@3240000"; /* ~ CertiKOS TEE */
 		spi4 = "/aon_spi@c260000";
 		spi6 = "/spi@3270000";
 		tegra-camera-rtcpu = "/rtcpu@b000000";
 		serial0 = "/serial@3100000";
 		serial1 = "/serial@3110000";
-		serial2 = "/serial@c280000";
+		// serial2 = "/serial@c280000"; /* ~ CertiKOS TEE */
 		serial3 = "/serial@3130000";
 		serial4 = "/serial@3140000";
 		serial5 = "/serial@3150000";
@@ -6200,7 +6200,7 @@
 		linux,phandle = <0x18b>;
 		phandle = <0x18b>;
 	};
-
+/* ~ CertiKOS TEE
 	spi@3240000 {
 		compatible = "nvidia,tegra186-spi";
 		reg = <0x00 0x3240000 0x00 0x10000>;
@@ -6243,7 +6243,7 @@
 			};
 		};
 	};
-
+*/
 	spi@3270000 {
 		compatible = "nvidia,tegra186-qspi";
 		reg = <0x00 0x3270000 0x00 0x10000>;
@@ -6427,7 +6427,7 @@
 		linux,phandle = <0x194>;
 		phandle = <0x194>;
 	};
-
+/* ~ CertiKOS TEE
 	serial@c280000 {
 		compatible = "nvidia,tegra186-hsuart";
 		iommus = <0x11 0x20>;
@@ -6448,7 +6448,7 @@
 		linux,phandle = <0x195>;
 		phandle = <0x195>;
 	};
-
+*/
 	serial@3130000 {
 		compatible = "nvidia,tegra186-hsuart";
 		iommus = <0x11 0x20>;
@@ -9795,6 +9795,11 @@
 			linux,phandle = <0xf5>;
 			phandle = <0xf5>;
 		};
+		thinros_ipc {
+			compatible = "certikos,thinros-ipc";
+			reg = <0x1 0x0 0x0 0x20000000>;
+			no-map;
+		};
 	};
 
 	timer {
@@ -10579,10 +10584,11 @@
 		compatible = "arm,cortex-a15-gic";
 		#interrupt-cells = <0x03>;
 		interrupt-controller;
-		reg = <0x00 0x3881000 0x00 0x1000 0x00 0x3882000 0x00 0x2000>;
+		reg = <0x0 0x3881000 0x0 0x1000 0x0 0x3882000 0x0 0x2000 0x0 0x3884000 0x0 0x2000 0x0 0x3886000 0x0 0x2000>;
 		status = "okay";
 		linux,phandle = <0x01>;
 		phandle = <0x01>;
+		interrupts = <1 9 0xf04>;
 	};
 
 	interrupt-controller@3000000 {
@@ -17348,7 +17354,7 @@
 		spi0 = "/spi@3210000";
 		spi1 = "/spi@c260000";
 		spi2 = "/spi@3230000";
-		spi3 = "/spi@3240000";
+		// spi3 = "/spi@3240000"; /* ~ CertiKOS TEE */
 		qspi6 = "/spi@3270000";
 		aon_spi = "/aon_spi@c260000";
 		tegra_pwm1 = "/pwm@3280000";
@@ -17361,7 +17367,7 @@
 		tegra_pwm8 = "/pwm@32f0000";
 		uarta = "/serial@3100000";
 		uartb = "/serial@3110000";
-		uartc = "/serial@c280000";
+		// uartc = "/serial@c280000"; /* ~ CertiKOS TEE */
 		uartd = "/serial@3130000";
 		uarte = "/serial@3140000";
 		uartf = "/serial@3150000";
EOF
