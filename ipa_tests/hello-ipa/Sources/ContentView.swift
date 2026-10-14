import SwiftUI

struct ContentView: View {
    var body: some View {
        ZStack {
            Color(uiColor: .systemBackground)
                .ignoresSafeArea()

            Text("Hallo IPA")
                .font(.system(size: 44, weight: .bold, design: .rounded))
                .foregroundStyle(.primary)
        }
    }
}
